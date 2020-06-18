/*
 * Copyright (C) 2006 - 2011  Simone Campanoni
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
#ifndef IR_VIRTUAL_MACHINE_H
#define IR_VIRTUAL_MACHINE_H

#include <xanlib.h>
#include <jitsystem.h>
#include <ir_optimizer.h>
#include <garbage_collector.h>
#include <framework_garbage_collector.h>

/**
 * \defgroup IRBACKEND IR backend
 * \ingroup IRLanguage
 *
 * IR code is managed through the IR backend here described.
 *
 * Every code outside this module is not aware on how machine code is produced or executed, they rely on both the IR execution model and the IR language.
 *
 * In order to introduce a new backend, a new module of this type has to be provided, which implemets every function defined here.
 *
 * Every backend has to declare the following data structures inside the file ir_virtual_machine_backend.h
 * <ul>
 * <li> IRVM_t
 * <li> t_jit_function
 * <li> IRVM_stackTrace
 * <li> IRVM_type
 * <li> t_thread_exception_infos
 * </ul>
 */

/**
 * \defgroup IRBACKEND_codegeneration Code generation
 * \ingroup IRBACKEND
 */

/**
 * \defgroup IRBACKEND_execution Code execution
 * \ingroup IRBACKEND
 */

/**
 * \defgroup IRBACKEND_exceptions Exception
 * \ingroup IRBACKEND
 */

#define OUT_OF_MEMORY_EXCEPTION 1

#define COMPILATION_RESULT_OK                   (1)
#define COMPILATION_RESULT_OVERFLOW             (0)
#define COMPILATION_RESULT_ARITHMETIC           (-1)
#define COMPILATION_RESULT_DIVISION_BY_ZERO     (-2)
#define COMPILATION_RESULT_COMPILE_ERROR        (-3)
#define COMPILATION_RESULT_OUT_OF_MEMORY        (-4)
#define COMPILATION_RESULT_NULL_REFERENCE       (-5)
#define COMPILATION_RESULT_NULL_FUNCTION        (-6)
#define COMPILATION_RESULT_CALLED_NESTED        (-7)
#define COMPILATION_RESULT_OUT_OF_BOUNDS        (-8)
#define COMPILATION_RESULT_UNDEFINED_LABEL      (-9)
#define COMPILATION_RESULT_CACHE_FULL           (-10000)

typedef struct {
    void *type;
} IRVM_type;

typedef struct {
    void *stack_trace;
} IRVM_stackTrace;

typedef struct {
    IRVM_stackTrace         *stackTrace;
    void                    *exception_thrown;
    JITBOOLEAN isConstructed;
} t_thread_exception_infos;

typedef struct {
    void                    *data;
} t_jit_function;

/**
 * \ingroup IRBACKEND
 * @brief IR machine configuration
 *
 * Information about the configuration of the IR machine
 */
typedef struct {
    JITBOOLEAN verbose;
    JITBOOLEAN tracer;
    JITUINT32 tracerOptions; 				/**< 0: all methods. 1: only program (no libraries). 2: only libraries.	*/
    JITINT32 profiler;
    JITUINT32 optimizations;                                /**< Optimization level to use in libiljitiroptimizer for the new generated code*/
    JITUINT32 libjitOptimizations;                          /**< Optimization level for the libjit library					*/
    JITBOOLEAN noExplicitGarbageCollection;
    JITBOOLEAN	noLinker;
    JITBOOLEAN debugExecution;
    JITBOOLEAN disableStaticConstructors;
    JITNUINT heapSize;
    struct {
        JITBOOLEAN dumpAssembly;
        FILE            *dumpFileIR;
        FILE            *dumpFileJIT;
        FILE            *dumpFileAssembly;
    } dumpAssembly;
    char *gcName;
    char *outputPrefix;
    JITBOOLEAN jit;
    JITBOOLEAN dla;
    JITBOOLEAN aot;
    JITBOOLEAN staticCompilation;
    JITBOOLEAN onlyPrecompilation;
    JITINT8 pgc;
} ir_system_behavior_t;

/**
 * \ingroup IRBACKEND
 *
 * IR Virtual Machine, which provides the backend.
 */
typedef struct IRVM_t {
    t_running_garbage_collector *gc;                                        /**< Running garbage collector of ILDJIT		*/
    ir_lib_t ir_system;                                                     /**< IR system                                          */
    ir_system_behavior_t behavior;                                          /**< Behavior parameters of the IR virtual machine	*/
    JITINT8     *linkerOptions;
    pthread_mutex_t vmMutex;                                                /**< Virtual Machine mutex				*/
    ir_optimizer_t optimizer;                                               /**< IR code optimizer					*/
    void    *data;

    JITINT32 		(*recompilerFunction)	(void *ilmethod, void *caller, ir_instruction_t *inst);                  /**< Function to invoke when a method should be recompiled	*/
    ir_method_t *           (*getIRMethod)		(IR_ITEM_VALUE method);
    void 			(*leaveExecution)	(JITINT32 exitCode);
    t_jit_function *        (*getJITMethod)		(IR_ITEM_VALUE method);
    IRVM_type *             (*lookupValueType)	(TypeDescriptor *type);
} IRVM_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup IRBACKEND
 *
 * Initialize the IR virtual machine <code> _this </code>.
 *
 * Function pointers given as input are functions that can be used by the backend for its implementation.
 *
 * @param _this IR virtual machine to initialize
 * @param lookupValueType Function that returns the backend-dependent type correspondent to the type specified as input
 * @param getIRMethod This function returns the IR method correspondent to the one specified as input.
 * @param leaveExecution This function shutdowns ILDJIT and it terminates its execution.
 * @param leaveExecutionFunctionName Name of the C function to call to leave the execution of the program.
 * @param getJITMethod This function returns the backend-dependent method correspondent to the one specified as input.
 * @param recompilerFunction This function represent the trampolines to call whenever a method specified as input needs to be compiled.
 */
void IRVM_init(IRVM_t *_this, t_running_garbage_collector *gc, IRVM_type * (*lookupValueType)(TypeDescriptor *type), ir_method_t * (*getIRMethod)(IR_ITEM_VALUE method), void (*leaveExecution)(JITINT32 exitCode), JITINT8 *leaveExecutionFunctionName, t_jit_function * (*getJITMethod)(IR_ITEM_VALUE method), JITINT32 (*recompilerFunction)(void *ilmethod, void *caller, ir_instruction_t *inst));

/**
 * \ingroup IRBACKEND
 *
 * Shutdown the IR virtual machine <code> _this </code>
 *
 * This function frees the memory used by the backend to both execute and compile code.
 *
 * @param _this IR virtual machine to consider
 */
void IRVM_shutdown (IRVM_t *_this);

/**
 * \ingroup IRBACKEND_execution
 *
 * Call the function <code> jitFunction </code> with the supplied arguments.
 * This method returns zero if an exception occurred.
 *
 * The parameter <code> args </code> is an array and each element of it is a pointer to one of the arguments.
 *
 * Moreover, <code> returnArea </code> points to a buffer to receive the return value.

 * This is the primary means for executing a function from ordinary C code without creating a closure first with <code> IRVM_getFunctionPointer </code>.
 *
 * Closures may not be supported on all platforms, but function application is guaranteed to be supported everywhere.
 *
 * Function applications acts as an exception blocker.
 * If any exceptions occur during the execution of <code> jitFunction </code>, they won't travel up the stack any further than this point.
 * This prevents ordinary C code from being accidentally presented with a situation that it cannot handle.
 *
 * This blocking protection is not present when a function is invoked via its closure.
 *
 * @param jitFunction Function to execute
 * @param returnArea Memory to use to store the return value of the invoked function
 */
JITINT32 IRVM_run (IRVM_t *self, t_jit_function *jitFunction, void **args, void *returnArea);

/**
 * \ingroup IRBACKEND_codegeneration
 * @brief Start the pre-compilation
 *
 * This function is called just before generating any machine code and before starting any execution.
 *
 * @param self IR virtual machine
 */
void IRVM_startPreCompilation (IRVM_t *self);

/**
 * \ingroup IRBACKEND_codegeneration
 * @brief Finish the pre-compilation
 *
 * This function is called just after all code generation performed before starting any execution of the generated code.
 *
 * @param self IR virtual machine
 */
void IRVM_finishPreCompilation (IRVM_t *self);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Translate the IR function <code> method </code> to an equivalent machine code and store it to <code> jitFunction </code>.
 *
 * @param _this IR virtual machine
 * @param method IR function to compile
 * @param jitFunction Memory used to compile <code> method </code>
 */
void IRVM_lowerMethod (IRVM_t *_this, ir_method_t *method, t_jit_function *jitFunction);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Make the function <code> method </code> previously compiled available for invocation and free the resources used for compilation.
 *
 * @param _this IR virtual machine
 * @param jitFunction Backend-dependent representation of <code> method </code>
 * @param method IR function to consider
 */
void IRVM_generateMachineCode (IRVM_t *_this, t_jit_function *jitFunction, ir_method_t *method);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * This function is called after all methods have been translated to the backend representation (by calling \ref IRVM_lowerMethod) and before any machine code is generated (by calling \ref IRVM_generateMachineCode).
 *
 * @param self IR virtual machine
 */
void IRVM_optimizeProgram (IRVM_t *self);

/**
 * \ingroup IRBACKEND
 *
 * Check if the backend-dependent function <code> function </code> is native (i.e., a C function) or if it has been generated by the compiler.
 *
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRVM_isANativeFunction (IRVM_t *self, t_jit_function *function);

/**
 * \ingroup IRBACKEND
 *
 * Return the entry point of the function.
 *
 * The entry point is the first memory address where the machine code of <code> function </code> has been stored.
 *
 * @param function Backend-dependent function
 * @return Entry point of <code> function </code>
 */
void * IRVM_getEntryPoint (IRVM_t *self, t_jit_function *function);

/**
 * \ingroup IRBACKEND
 *
 * Set the entry point of a native function (i.e., a C function) to the backend-dependent function <code> function </code>.
 *
 * @param function Backend-dependent function
 * @param nativeEntryPoint Entry point of a native function
 */
void IRVM_setNativeEntryPoint (IRVM_t *self, t_jit_function *function, void *nativeEntryPoint);

/**
 * \ingroup IRBACKEND
 *
 * Destroy the compiled function <code> function </code>.
 *
 * This method frees the memory used to handle <code> function </code> without freeing the data structure <code> t_jit_function </code>.
 *
 * @param function Function to destroy.
 */
void IRVM_destroyFunction (IRVM_t *self, t_jit_function *function);

/**
 * \ingroup IRBACKEND_exceptions
 *
 * Throw the runtime object pointed by <code> object </code> previously allocated.
 *
 * @param object Object to throw
 */
void IRVM_throwException (IRVM_t *self, void *object);

/**
 * \ingroup IRBACKEND_exceptions
 *
 * Create an exception object of type <code> exceptionType </code> and throw it.
 *
 * @param exceptionType Type of the exception to throw
 */
void IRVM_throwBuiltinException (IRVM_t *self, JITUINT32 exceptionType);

/**
 * \ingroup IRBACKEND
 *
 * Convert a compiled function into a closure that can be called directly from C.
 *
 * Returns NULL if out of memory, or if closures are not supported on this platform.
 *
 * If the function has not been compiled yet, then this will return a pointer to a redirector that will arrange for the function to be compiled on-demand when it is called.
 *
 * @param self IR virtual machine
 * @param jitFunction Compiled function to consider
 */
void * IRVM_getFunctionPointer (IRVM_t *self, t_jit_function *jitFunction);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Translate the IR type <code> irvmType </code> to the backend-dependent type and store it inside the data structure <code> irvmType </code>.
 *
 * In case there is a description of IR type <code> irvmType </code> that is input language-dependent, this description is pointed by <code> ilClass </code>
 *
 * The parameter <code> ilClass </code> can be used for data structure types.
 *
 * @param self IR virtual machine
 * @param irType IR type to consider
 * @param irvmType Backend-dependent type equivalent to <code> irType </code>
 * @param ilClass Input language dependent type of <code> irType </code>
 */
void IRVM_translateIRTypeToIRVMType (IRVM_t *self, JITUINT16 irType, IRVM_type *irvmType, TypeDescriptor *ilClass);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Return the number of bytes used to store variables of type <code> irvmType </code> at most.
 *
 * @param irvmType Backend-dependent type to consider
 * @return Number of bytes used to store variables of type <code> irvmType </code>
 */
JITUINT32 IRVM_getIRVMTypeSize (IRVM_t *self, IRVM_type *irvmType);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Create a backend-dependent structure type of size <code> structureSize </code> aligned in <code> alignment </code> bytes and store it to <code> irvmType </code>.
 *
 * @param irvmType Backend-dependent type
 * @param structureSize Number of bytes of the structure to create
 * @param alignment Alignment of the structure to create
 */
void IRVM_setIRVMStructure (IRVM_t *self, IRVM_type *irvmType, JITNINT structureSize, JITNINT alignment);

/**
 * \ingroup IRBACKEND_execution
 *
 * Return the IR function that is running at the time this method is called
 *
 * @param self IR virtual machine
 * @return IR method in execution
 */
ir_method_t * IRVM_getRunningMethod (IRVM_t *self);

/**
 * \ingroup IRBACKEND_execution
 *
 * Store to <code> method </code> the method currently in execution; the instruction of <code> method </code> in execution is stored to <code> instr </code>.
 *
 * @param self IR virtual machine
 * @param method Pointer to the memory where the method in execution will be stored
 * @param instr Pointer to the memory where the instruction in execution of the method <code> method </code> will be stored
 */
void IRVM_getRunningMethodCallerInstruction (IRVM_t *self, ir_method_t **method, ir_instruction_t **instr);

/**
 * \ingroup IRBACKEND_execution
 *
 * Return the IR function that is currently running, which is the caller of <code> m </code> inside the current call stack.
 *
 * @param self IR virtual machine
 * @param m Method in execution, which has been called by the returned method
 * @return IR method in execution
 */
ir_method_t * IRVM_getCallerOfMethodInExecution (IRVM_t *self, ir_method_t *m);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Create a new empty backend-dependent function with signature specified by <code> backendFunction </code>.
 *
 * Notice that this means that the signature has to be created before calling this function.
 *
 * Signatures can be created by calling <code> IRVM_translateIRMethodSignatureToBackendSignature </code> like in the following case:
 *
 * @code
 * IRVM_translateIRMethodSignatureToBackendSignature(this, IRSignature, backendFunction);
 * IRVM_newBackendMethod(this, method, backendFunction, JITTRUE, JITTRUE);
 * @endcode
 *
 * @param _this IR virtual machine
 * @param method Pointer to set as metadata to <code> backendFunction </code>
 * @param backendFunction Memory to store the new backend-dependent function
 * @param isRecompilable JITTRUE or JITFALSE
 * @param isCallableExternally JITTRUE if the created method can be called from outside the generated machine code, JITFALSE otherwise.
 */
void IRVM_newBackendMethod (IRVM_t *_this, void *method, t_jit_function *backendFunction, JITBOOLEAN isRecompilable, JITBOOLEAN isCallableExternally);

/**
 * \ingroup IRBACKEND_exceptions
 *
 * Set the builtin exception handler <code> exceptionHandler </code> for the current thread.
 *
 * @param _this IR virtual machine
 * @param exceptionHandler Handler to set
 */
void IRVM_setExceptionHandler(IRVM_t *_this, void * (*exceptionHandler)(int exception_type));

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Translate the IR signature <code> ir_signature </code> to the backend-dependent signature and return it.
 *
 * @param _this IR virtual machine
 * @param ir_signature IR signature to consider
 * @param backendFunction Backend function to store the new created signature
 */
void IRVM_translateIRMethodSignatureToBackendSignature (IRVM_t *_this, ir_signature_t *ir_signature, t_jit_function *backendFunction);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Check if the backend-dependent signature has been translated.
 *
 * @param backendFunction Backend-dependent function to consider
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRVM_hasTheSignatureBeenTranslated (IRVM_t *self, t_jit_function *backendFunction);

/**
 * \ingroup IRBACKEND_codegeneration
 *
 * Check if the compilation is available for the underlying platform.
 *
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRVM_isCompilationAvailable (IRVM_t *self);

/*********** Stack trace ******************/
IRVM_stackTrace * IRVM_getStackTrace (void);
void IRVM_destroyStackTrace (IRVM_stackTrace *stack);
JITUINT32 IRVM_getStackTraceOffset (IRVM_t *IRVM, IRVM_stackTrace *stack, JITUINT32 position);
void * IRVM_getStackTraceFunctionAt (IRVM_t *IRVM, IRVM_stackTrace *stack, JITUINT32 position);
JITUINT32 IRVM_getStackTraceSize (IRVM_stackTrace *stack);

void libirvirtualmachineCompilationFlags (char *buffer, int bufferLength);

void libirvirtualmachineCompilationTime (char *buffer, int bufferLength);

char * libirvirtualmachineVersion (void);

#ifdef __cplusplus
};
#endif

#endif
