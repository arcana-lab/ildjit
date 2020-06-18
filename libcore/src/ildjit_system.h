/*
 * Copyright (C) 2006 - 2012 Campanoni Simone
 *
 * iljit - This is the core of the ILDJIT compilation framework
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
/**
 * @file system_manager.h
 */
#ifndef ILDJIT_SYSTEM_H
#define ILDJIT_SYSTEM_H

#include <time.h>
#include <compiler_memory_manager.h>
#include <dla.h>
#include <ir_virtual_machine.h>
#include <garbage_collector.h>
#include <cli_manager.h>
#include <manfred.h>

// My header
#include <translation_pipeline.h>
#include <static_memory.h>
// End

typedef struct {
    JITBOOLEAN	cacheCodeGeneration;
    XanList		*cachedMethods;
} CodeGenerator;

/**
 * @brief Exception-System struct
 *
 * This struct mantains all the informations that are useful in managing the exception infos.
 */
typedef struct {
    TypeDescriptor          *_System_Exception_ID;
    TypeDescriptor          *_System_ArithmeticException_ID;
    TypeDescriptor          *_System_ArrayTypeMismatchException_ID;
    TypeDescriptor          *_System_DivideByZeroException_ID;
    TypeDescriptor          *_System_ExecutionEngineException_ID;
    TypeDescriptor          *_System_FieldAccessException_ID;
    TypeDescriptor          *_System_IndexOutOfRangeException_ID;
    /*	TypeDescriptor		*_System_InvalidAddressException_ID;*/
    TypeDescriptor          *_System_InvalidCastException_ID;
    TypeDescriptor          *_System_InvalidOperationException_ID;
    TypeDescriptor          *_System_MethodAccessException_ID;
    TypeDescriptor          *_System_MissingFieldException_ID;
    TypeDescriptor          *_System_MissingMethodException_ID;
    TypeDescriptor          *_System_NullReferenceException_ID;
    TypeDescriptor          *_System_OverflowException_ID;
    TypeDescriptor          *_System_OutOfMemoryException_ID;
    /*	TypeDescriptor		*_System_SecurityException_ID;*/
    TypeDescriptor          *_System_StackOverflowException_ID;
    TypeDescriptor          *_System_TypeLoadException_ID;
    TypeDescriptor          *_System_ArgumentException_ID;
    /*	TypeDescriptor		*_System_VerificationException_ID; */
    TypeDescriptor          *_System_ArgumentOutOfRangeException_ID;

    /* a permanent instance of OutOfMemoryException */
    void                    *_OutOfMemoryException;
} t_exception_system;

typedef struct {
    Method				entry_point;		/**< Entry point of the CIL bytecode					*/
    Method				wrapperEntryPoint;
    t_thread_exception_infos	*thread_exception_infos;
    JITINT32			result;			/**< Result value of the CIL bytecode					*/
    JITINT32			cil_opcodes_decoded;	/**< Total number of CIL opcodes translated to the IR instructions.	*/
    JITBOOLEAN			disableExecution;	/**< Enable or disable the execution of the input program		*/
    JITBOOLEAN			spawnNewThreadAsMainCodeExecutor;
    JITBOOLEAN			enableMachineCodeGeneration;
} t_program;

typedef struct {
    char                    **argv;
    JITINT32 		argc;
    void			*stringArray;
} t_arg;

typedef struct {
    XanList				*plugins;
    t_running_garbage_collector	gc;
} t_garbage_collectors;

typedef struct {
    struct timespec time;
    struct timeval wallTime;
} ProfileTime;

/**
 * Profile information
 *
 * This is the structure contains every information about the execution of the CIL program.
 */
typedef struct {
    struct {
        JITFLOAT32 execution_time;
        JITFLOAT32 dla_time;
        JITFLOAT32 trampolines_time;                    /**< Time spent in the trampolines						*/
        JITFLOAT32 cil_loading_time;                    /**< Time spent reading CIL files and decoding them				*/
        JITFLOAT32 bootstrap_time;                      /**< Time spent at bootstrap time						*/
        JITFLOAT32 total_time;                          /**< Total time spent by ILDJIT to execute the CIL program given as input	*/
        JITFLOAT32 cil_ir_translation_time;             /**< Time spent translating the CIL methods into IR one				*/
        JITFLOAT32 ir_optimization_time;                /**< Time spent optimizing IR methods						*/
        JITFLOAT32 ir_machine_code_translation_time;    /**< Time spent translating the IR methods to the machine code			*/
        JITFLOAT32 static_memory_time;                  /**< Time spent within the static memory phase of the pipeliner			*/
        JITFLOAT32 average_pipeline_latency;            /**< Average of the latency due to the compilation pipeline			*/
    } wallTime;
    JITFLOAT32 total_time;                                  /**< Total time spent by ILDJIT to execute the CIL program given as input	*/
    JITFLOAT32 bootstrap_time;                              /**< Time spent at bootstrap time						*/
    JITFLOAT32 cil_loading_time;                            /**< Time spent reading CIL files and decoding them				*/
    JITFLOAT32 cil_ir_translation_time;                     /**< Time spent translating the CIL methods into IR one				*/
    JITFLOAT32 dla_time;                                    /**< Time spent by DLA								*/
    JITFLOAT32 ir_optimization_time;                        /**< Time spent optimizing IR methods						*/
    JITFLOAT32 ir_machine_code_translation_time;            /**< Time spent translating the IR methods to the machine code			*/
    JITFLOAT32 execution_time;                              /**< Time spent executing the machine code					*/
    JITFLOAT32 static_memory_time;                          /**< Time spent within the static memory phase of the pipeliner			*/
    JITFLOAT32 trampolines_time;                            /**< Time spent in the trampolines						*/
    JITFLOAT32 average_pipeline_latency;                    /**< Average of the latency due to the compilation pipeline			*/
    JITUINT32 trampolinesTaken;                             /**< Number of trampolines taken by the execution				*/
    JITUINT32 trampolinesTakenBeforeEntryPoint;             /**< Number of trampolines taken before starting the execution of the entry point*/
    ProfileTime startTime;                                  /**< Start time of iljit program						*/
    ProfileTime startExecutionTime;                         /**< When iljit starts executing the code dynamically generated.		*/
} t_profiler;

/**
 * @brief System struct
 *
 * This struct fills up the informations of the CIL program given as input
 */
typedef struct t_system {
    XanList                         *CLRPlugins;		/**< CLR plugins found in the system			*/
    XanList                         *plugins;		/**< Decoder Plugins of the system			*/
    t_garbage_collectors		garbage_collectors;	/**< Garbage collectors found in the system		*/
    t_program			program;		/**< Information about the CIL program			*/
    t_arg				arg;			/**< The arguments of the CIL program			*/
    t_profiler			profiler;		/**< Time Profiler					*/
    TranslationPipeline		pipeliner;		/**< Translation pipeliner				*/
    manfred_t			manfred;		/**< AOT compiler                                       */
    StaticMemoryManager		staticMemoryManager;	/**< Manager of the static memory			*/
    CodeGenerator			codeGenerator;		/**< Module in charge to generate machine code.		*/
    MemoryManager			compilerMemoryManager;	/**< Manager of the memory used by the compiler.        */
    DLA_t				DLA;			/**< Dynamic Look Ahead compiler                        */
    IRVM_t				IRVM;			/**< IR virtual machine					*/
    CLIManager_t			cliManager;		/**< Manager of the CLI architecture			*/
    char				*cilLibraryPath;	/**< CIL library paths					*/
    JITINT8				*machineCodeLibraryPath;/**< Machine code library paths				*/
    char 				*programName;		/**< Name of the program				*/
    t_exception_system		exception_system;	/**< THE EXCEPTION SYSTEM                               */
} t_system;

t_binary_information * ILDJIT_loadAssembly (JITINT8 *assemblyName);

void * ILDJIT_boxNullValueType (TypeDescriptor *valueType);

void * ILDJIT_boxNullPrimitiveType (JITUINT32 IRType);

char * ILDJIT_getPrefix (void);

JITINT32 ILDJIT_main (JITINT32 argc, char **argv, JITBOOLEAN multiapp);

#endif
