/*
 * Copyright (C) 2006 - 2012  Simone Campanoni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABIRITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/**
 * @file ir_method.h
 */
#ifndef IR_METHOD_H
#define IR_METHOD_H

#include <assert.h>
#include <xanlib.h>
#include <metadata/metadata_types.h>
#include <ir_language.h>
#include <iljit-utils.h>

/**
 * \defgroup IRMETHOD Code API
 * \ingroup IRLanguage
 *
 * IR methods can be managed by the API described within this module.
 *
 * The modules \ref IRMETHOD_BasicBlock , \ref IRMETHOD_Instruction , \ref IRMETHOD_DataType , \ref IRMETHOD_Method , \ref IRMETHOD_Program and \ref IRMETHOD_Loop give you the ability to get information about basic blocks, instructions, data types, methods, the input program and loops respectively.
 *
 * The modules \ref IRMETHOD_ModifyInstructions , \ref IRMETHOD_ModifyMethods and \ref IRMETHOD_ModifyProgram give you the ability to modify single IR instructions, IR methods and the input program respectively.
 *
 * The module \ref IRMETHOD_CodeGeneration gives you the ability to control how to and when to generate IR or machine code.
 * Moreover, it allows you to execute the produced code.
 *
 * Dependences across instructions, basic blocks and methods can be found in the module \ref IRMETHOD_Dependences.
 *
 * In module \ref IRMETHODMETADATA you can find information about how to exploit and extend high level information for IR methods.
 *
 * Functions that are useful to dump information about IR code are grouped in module \ref IRMETHOD_Dump.
 *
 * Functions to handle IR symbols are grouped in module \ref IRSYMBOLS
 *
 */

/**
 * \defgroup IRMETHOD_MethodBody Body
 * \ingroup IRMETHOD_Method
 *
 * Information about the body of methods can be get through the API described in this section.
 */

/**
 * \defgroup IRMETHOD_MethodLabels Labels
 * \ingroup IRMETHOD_Method
 */

/**
 * \defgroup IRMETHOD_Method_Signature Signature
 * \ingroup IRMETHOD_Method
 *
 * Information about signature of IR methods is provided by this section.
 */

/**
 * \defgroup IRMETHOD_InstructionsNavigation Code navigation
 * \ingroup IRMETHOD_Method
 *
 * Information about how to navigate through the code of a single method is provided in this section.
 */

/**
 * \defgroup IRMETHOD_DataType Data type
 * \ingroup IRMETHOD_CodeAnalysis
 *
 * Information about single data types can be found within this section.
 */

/**
 * \defgroup IRMETHOD_Instruction Instruction
 * \ingroup IRMETHOD_CodeAnalysis
 *
 * Information about instructions within IR methods can be found using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_BasicBlock Basic blocks
 * \ingroup IRMETHOD_Method
 *
 * Information about the basic blocks within IR methods can be found using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_InstructionParameters Instruction parameters
 * \ingroup IRMETHOD_Instruction
 *
 * Information about parameters of IR instructions within IR methods can be found using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_InstructionCallParameters Instruction call parameters
 * \ingroup IRMETHOD_Instruction
 *
 * Information about call parameters of IR instructions used within IR methods can be found using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_InstructionUses Variables used
 * \ingroup IRMETHOD_Instruction
 *
 */

/**
 * \defgroup IRMETHOD_InstructionDefine Variables defined
 * \ingroup IRMETHOD_Instruction
 *
 * Information about variables set by IR instructions can be found using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_InstructionCalledMethod Called methods
 * \ingroup IRMETHOD_Instruction
 *
 * Information about possible callee of the instruction can be found using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_InstructionDominance Navigation through the instruction-dominance relation
 * \ingroup IRMETHOD_InstructionsNavigation
 *
 * Information about pre- and post dominance between instructions can be found by using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_InstructionCFG Navigation through the CFG
 * \ingroup IRMETHOD_InstructionsNavigation
 *
 * Information about successor/predecessor relation between instructions in the CFG is described in this section.
 */

/**
 * \defgroup IRMETHOD_InstructionSequence Navigation through the IR instruction storing sequence
 * \ingroup IRMETHOD_InstructionsNavigation
 *
 * Information about the way IR instructions are stored inside the method can be found using the API described in this section.
 */

/**
 * \defgroup IRMETHOD_Circuit Circuits
 * \ingroup IRMETHOD_Method
 *
 * Information about loops of IR methods is provided within this section.
 */

/**
 * \defgroup IRMETHOD_Method_Instructions Instructions
 * \ingroup IRMETHOD_MethodBody
 *
 * Information about instructions used within single IR methods is provided by this section.
 */

/**
 * \defgroup IRMETHOD_Method_Variables Variables
 * \ingroup IRMETHOD_MethodBody
 *
 * Information about variables used within single IR methods is provided by this section.
 */

/**
 * \defgroup IRMETHOD_Circuit_Backedge Back-edge
 * \ingroup IRMETHOD_Circuit
 *
 * Information about loop backedges is provided by this section.
 */

/**
 * \defgroup IRMETHOD_Circuit_Induction_Variables Induction variables
 * \ingroup IRMETHOD_Circuit
 *
 * Information about induction variables of loops is provided by this section.
 */

/**
 * \defgroup IRMETHOD_Circuit_Invariants Invariants
 * \ingroup IRMETHOD_Circuit
 *
 * Information about loop invariants is provided by this section.
 */

/**
 * \defgroup IRMETHOD_Method Method
 * \ingroup IRMETHOD_CodeAnalysis
 *
 * Information about single IR methods is provided within this section.
 */

/**
 * \defgroup IRMETHOD_Method_entryPoint Entry point
 * \ingroup IRMETHOD_Method
 */

/**
 * \defgroup IRMETHOD_Program Program
 * \ingroup IRMETHOD_CodeAnalysis
 *
 * Information about the program that is running is provided within this section.
 */

/**
 * \defgroup IRMETHOD_Loop Loop
 * \ingroup IRMETHOD_CodeAnalysis
 *
 * Information about loops of the program is provided within this section.
 */

/**
 * \defgroup IRMETHOD_IRCodeCache IR code cache
 * \ingroup IRMETHOD_Program
 *
 * The functions are related to the code cache where IR code can be stored.
 */

/**
 * \defgroup IRMETHOD_DataTypeTypeHierarchy Type hierarchy
 * \ingroup IRMETHOD_DataType
 */

/**
 * \defgroup IRMETHOD_DataTypeGlobal Global
 * \ingroup IRMETHOD_DataType
 */

/**
 * \defgroup IRMETHOD_DataTypeGlobalStatic Static memory
 * \ingroup IRMETHOD_DataTypeGlobal
 */

/**
 * \defgroup IRMETHOD_DataTypeConstant Constants
 * \ingroup IRMETHOD_DataType
 */

/**
 * \defgroup IRMETHOD_DataTypeVariable Variables
 * \ingroup IRMETHOD_DataType
 */

/**
 * \defgroup IRMETHOD_DataTypeSymbol Symbols
 * \ingroup IRMETHOD_DataType
 */

/**
 * \defgroup IRMETHOD_DataTypeMethod Method
 * \ingroup IRMETHOD_DataType
 */

/**
 * \defgroup IRMETHOD_DataTypeSize Size
 * \ingroup IRMETHOD_DataType
 */

/**
 * \defgroup IRMETHOD_CodeAnalysis Code analysis
 * \ingroup IRMETHOD
 *
 * Information about how to analyze IR code is provided by this section.
 */

/**
 * \defgroup IRMETHOD_CodeTransformation Code transformation
 * \ingroup IRMETHOD
 *
 * Information about how to transform IR code is provided by this section.
 */

/**
 * \defgroup IRMETHOD_CodeTransformationDataType Data type
 * \ingroup IRMETHOD_CodeTransformation
 */

/**
 * \defgroup IRMETHOD_CodeTransformationDataTypeGlobal Global
 * \ingroup IRMETHOD_CodeTransformationDataType
 */

/**
 * \defgroup IRMETHOD_CodeGeneration Code generation
 * \ingroup IRMETHOD
 *
 * Information about code generation is provided within this section.
 */

/**
 * \defgroup IRMETHOD_Execution Code execution
 * \ingroup IRMETHOD
 *
 * Information about code that is running through ILDJIT is provided within this section.
 */

/**
 * \defgroup IRMETHOD_Linkage Code linking
 * \ingroup IRMETHOD
 *
 * Information about how to link code is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyInstructions Instruction
 * \ingroup IRMETHOD_CodeTransformation
 *
 * Information about how to modify IR instructions is provided within this section.
 */

/**
 * \defgroup IRMETHOD_ModifyInstructionParameters Instruction parameters
 * \ingroup IRMETHOD_ModifyInstructions
 *
 * Information about how to modify parameters of IR instructions is provided in this section.
 */

/**
 * \defgroup IRMETHOD_ModifyInstructionParameters_cp Copy
 * \ingroup IRMETHOD_ModifyInstructionParameters
 */

/**
 * \defgroup IRMETHOD_ModifyInstructionParameters_set Set
 * \ingroup IRMETHOD_ModifyInstructionParameters
 */

/**
 * \defgroup IRMETHOD_ModifyInstructionParameters_sub Substitution
 * \ingroup IRMETHOD_ModifyInstructionParameters
 */

/**
 * \defgroup IRMETHOD_ModifyInstructionCallParameters Instruction call parameters
 * \ingroup IRMETHOD_ModifyInstructions
 *
 * Information about how to modify call parameters of IR instructions is provided in this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethods Method
 * \ingroup IRMETHOD_CodeTransformation
 *
 * Information about how to modify IR methods is provided within this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodBody Body
 * \ingroup IRMETHOD_ModifyMethods
 */

/**
 * \defgroup IRMETHOD_ModifyMethodBodyInstructions Instructions
 * \ingroup IRMETHOD_ModifyMethodBody
 */

/**
 * \defgroup IRMETHOD_ModifyVariables Variables
 * \ingroup IRMETHOD_ModifyMethodBody
 *
 * Information about how to modify IR variables is provided within this section.
 */

/**
 * \defgroup IRMETHOD_ModifyLabels Labels
 * \ingroup IRMETHOD_ModifyMethods
 *
 * Information about how to modify IR labels is provided within this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsAddingInstructions Adding new instructions
 * \ingroup IRMETHOD_ModifyMethodBodyInstructions
 *
 * Information about how to add new instructions to IR methods is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsAddingCallInstructions Adding call instructions
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 *
 * Information about how to add new call instructions to IR methods is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsAddingBranchInstructions Adding branch instructions
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 *
 * Information about how to add new branch instructions to IR methods is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsAddingLabelInstructions Adding label instructions
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 *
 * Information about how to add new label instructions to IR methods is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsDeletingInstructions Delete instructions
 * \ingroup IRMETHOD_ModifyMethodBodyInstructions
 *
 * Information about how to delete instructions that belong to a given IR method is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsMovingInstructions Moving instructions
 * \ingroup IRMETHOD_ModifyMethodBodyInstructions
 *
 * Information about how to move instructions that belong to a given IR method is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsModifyingInstructions Multiple instructions
 * \ingroup IRMETHOD_ModifyMethodBodyInstructions
 *
 * Information about how to modify multiple instructions alltogether is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyMethodsSignature Signature
 * \ingroup IRMETHOD_ModifyMethods
 *
 * Information about how to modify a signature of a given method is provided by this section.
 */

/**
 * \defgroup IRMETHOD_ModifyProgram Program
 * \ingroup IRMETHOD_CodeTransformation
 *
 * Information about how to modify the input program is provided within this section.
 */

/**
 * \defgroup IRMETHOD_Dependences Dependences
 * \ingroup IRMETHODMETADATA
 *
 * Information about dependences between instructions is provided within this section.
 */

/**
 * \defgroup IRMETHOD_DataDependences Data dependences
 * \ingroup IRMETHOD_Dependences
 *
 */

/**
 * \defgroup IRMETHOD_DataDependencesAdd Add
 * \ingroup IRMETHOD_DataDependences
 *
 */

/**
 * \defgroup IRMETHOD_DataDependencesRemove Remove
 * \ingroup IRMETHOD_DataDependences
 *
 */

/**
 * \defgroup IRMETHOD_ControlDependences Control dependences
 * \ingroup IRMETHOD_Dependences
 *
 */

/**
 * \defgroup IRMETHOD_Method_VariablesAlias Memory aliases
 * \ingroup IRMETHOD_Dependences
 *
 * Information about memory aliases between variables can be found using the API provided by this section.
 */

/**
 * \defgroup IRMETHOD_Dump Code dumping
 * \ingroup IRMETHOD
 *
 * Functions that are useful for dumping information about the IR code are grouped in this section.
 */

/**
 * \defgroup IRMETHODMETADATA Metadata
 * \ingroup IRMETHOD_CodeAnalysis
 *
 * IR methods and IR instructions may include metadata information that is described in this section.
 */

/**
 * \defgroup IRMETHODMETADATA_DataFlow Data flow analysis
 * \ingroup IRMETHODMETADATA
 *
 * Information about data flow analysis can be found using the API described in this section.
 */

/**
 * \defgroup IRGLOBALS Globals
 * \ingroup IRMETHOD
 */

/**
 * \defgroup IRSYMBOLS Symbols
 * \ingroup IRMETHOD
 *
 * In order to have IR code that is position independent, symbols are used.
 *
 * Position independent IR code can be written to a file and read back in successive executions of ILDJIT.
 *
 * For example, instead of having a specific pointer value, a symbol is used instead.
 * Instead of having code that cannot be written to a file and read it back in another ILDJIT run (because the memory pointed by 0xDECAFBAD will be not valid anymore)
 * @code
 * v5 = IRLOADREL(0xDECAFBAD, 0);
 * @endcode
 * the following one can be created:
 * @code
 * v5 = IRLOADREL(SYMBOL 2 5, 0);
 * @endcode
 *
 * A symbol has a type called @c tag.
 * The unique identificator of a symbol is the pair (symbolValue, symbolTag).
 *
 * For example, the symbol 5 with tag 2 is the one of the previous example.
 *
 * This section describes the API to generate IR code with symbols.
 *
 * Next: \ref PLATFORM_API
 */

/**
 * \defgroup IRSYMBOLS_register Symbol registration
 * \ingroup IRSYMBOLS
 */

/**
 * \defgroup IRSYMBOLS_IO Symbols to files
 * \ingroup IRSYMBOLS
 */

/**
 * \defgroup IRSYMBOLS_Create Create symbols
 * \ingroup IRSYMBOLS
 */

/**
 * \defgroup IRSYMBOLS_Resolve Resolve symbols
 * \ingroup IRSYMBOLS
 */

/**
 * \defgroup IRSYMBOLS_Use Use symbols
 * \ingroup IRSYMBOLS
 */

/**
 * \ingroup IRMETHOD
 * @def IR_ITEM_VALUE
 * @brief The type of each non-float IR item.
 */
#define IR_ITEM_VALUE JITUINT64

/**
 * \ingroup IRMETHOD
 * @def IR_ITEM_FVALUE
 * @brief The type of each float IR item.
 */
#define IR_ITEM_FVALUE JITFLOAT64

/**
 * \ingroup IRMETHOD
 * @def DEP_RAW
 * @brief Data dependence of type Read After Write.
 */
#define DEP_RAW         0x1

/**
 * \ingroup IRMETHOD
 * @def DEP_WAR
 * @brief Data dependence of type Write After Read.
 */
#define DEP_WAR         0x2

/**
 * \ingroup IRMETHOD
 * @def DEP_WAW
 * @brief Data dependence of type Write After Write.
 */
#define DEP_WAW         0x4

/**
 * \ingroup IRMETHOD
 * @def DEP_MRAW
 * @brief Possible data dependence of type Read After Write through memory.
 */
#define DEP_MRAW        0x8

/**
 * \ingroup IRMETHOD
 * @def DEP_MWAR
 * @brief Possible data dependence of type Write After Read through memory.
 */
#define DEP_MWAR        0x10

/**
 * \ingroup IRMETHOD
 * @def DEP_MWAW
 * @brief Possible data dependence of type Write After Write through memory.
 */
#define DEP_MWAW        0x20

/**
 * \ingroup IRMETHOD
 * @def DEP_FIRSTMEMORY
 * @brief The first memory dependence.
 *
 * Every dependence that's greater or equal is a data dependence through memory.
 * Every dependence that's smaller does not involve memory
 */
#define DEP_FIRSTMEMORY         DEP_MRAW

/**
 * \ingroup IRMETHOD
 * @brief A basic block that contains IR instructions.
 */
typedef struct {
    JITUINT32 startInst;	/**< Position of the first instruction contained inside the basic block	*/
    JITUINT32 endInst;	/**< Position of the last instruction contained inside the basic block	*/
    JITUINT32 pos;		/**< Position of the basic block					*/
} IRBasicBlock;

/* Define the IRProfilerTime structure to store the time */
#define IRProfileTime struct timespec

/**
 * \ingroup IRMETHOD
 * @brief An induction variable.
 *
 * An induction variable can be computed by the following expression:
 * @code
 * a + b * i + pow(c, i)
 * @endcode
 * where <code> i </code> is the basic induction variable from which the current one is derived.
 */
typedef struct {
    JITUINT32 ID;   /**< ID of the induction variable.                                              */
    JITUINT32 i;    /**< ID of the basic induction variable from which this variable is derived.    */

    struct {
        JITUINT32 type; /**< The type of the term 'a':
		                 *    () IROFFSET : 'a' is a variable;
		                 *    () -IROFFSET: 'a' is a negated variable;
		                 *    () IRINT64 : 'a' is a constant value.
				 */
        JITINT64 value; /**< If 'a' is a variable, stores its ID, otherwise
		                 *   stores the constant value of 'a'.
				 */
    } a;	/**< The circuit invariant term 'a' 	*/

    struct {
        JITUINT32 type; /**< The type of the term 'b':
		                 *    () IROFFSET : 'b' is a variable;
		                 *    () -IROFFSET: 'b' is a negated variable;
		                 *    () IRINT64 : 'b' is a constant value.
				 */
        JITINT64 value; /**< If 'b' is a variable, stores its ID, otherwise
		                 *   stores the constant value of 'b'.
				 */
    } b; 	/**< The circuit invariant term 'b'  	*/

    struct {
        JITUINT32 type; /**< The type of the term 'c':
		                 *    () IROFFSET : 'b' is a variable;
		                 *    () -IROFFSET: 'b' is a negated variable;
		                 *    () IRINT64 : 'b' is a constant value.
				 */
        JITINT64 value; /**< If 'c' is a variable, stores its ID, otherwise
		                 *   stores the constant value of 'c'.
				 */
    } c; 	/**< The circuit invariant term 'c'  	*/

    /** Bitmap of coordinated induction variables introduced by strength reduction. */
    JITNUINT *coordinated;
} induction_variable_t;

typedef struct _IrMetadata {
    struct _IrMetadata	*next;
    JITUINT64		type;
    void			*data;
} IrMetadata;

/**
 * \ingroup IRMETHOD
 * @brief An ir value holds either a float or non-float value.
 */
typedef union {
    IR_ITEM_VALUE v;                                /**< Value of the parameter. If the parameter is a variable, then the value is the identification of the variable	*/
    IR_ITEM_FVALUE f;                               /**< Floating point value of the parameter. This field has to be considered only if the parameter is a constant of type IRFLOAT32, IRFLOAT64 or IRNFLOAT.	*/
} ir_value_t;

/**
 * \ingroup IRSYMBOLS
 * @brief An IR symbol
 *
 * An IR symbol defines a symbol constant that can be used as parameter for an instruction.
 */
typedef struct _ir_symbol_t {
    ir_value_t 	**value;
    JITUINT32	symbolIdentifier;
    ir_value_t (*resolve)(struct _ir_symbol_t *symbol);

    void (*serialize)(struct _ir_symbol_t *symbol, FILE *fileToWrite);
    void (*dump)(struct _ir_symbol_t *symbol, FILE *fileToWrite);
    /** Is the unique identifier of the symbol. Can be used to refer a symbol in serialization deserialization process*/
    JITUINT32 id;
    /** The Type of the symbol.	*/
    JITUINT32 tag;
    void *data;
} ir_symbol_t;

/**
 * \ingroup IRGLOBALS
 * @brief A single global
 *
 */
typedef struct {
    JITINT8		*name;		/**< Name of the global.						*/
    JITUINT16	IRType;		/**< IR type of the global. IRVALUETYPE is used for generic blobs.	*/
    JITUINT64	size;		/**< Number of bytes of the global.					*/
    JITBOOLEAN	isConstant;	/**< Whether the global can change value or not.			*/
    void		*initialValue;	/**< Memory to read to define the initial value of the global.		*/
} ir_global_t;

/**
 * \ingroup IRSYMBOLS
 * @brief Static memory
 *
 * Description of a static memory.
 */
typedef struct {
    ir_symbol_t	*s;
    ir_global_t	*global;
    void		*allocatedMemory;
    JITINT8		*name;
    JITUINT64	size;
} ir_static_memory_t;

/**
 * \ingroup IRMETHOD
 * @brief The type of an IR symbol contains functions to operate on it.
 */
typedef struct _IRSymbolType {
    /** Resolve symbol constant to actual value	*/
    ir_value_t (*resolve)(struct _ir_symbol_t *symbol);
    /** Serialize Symbol to a stream	*/
    void (*serialize)(struct _ir_symbol_t *symbol, void **mem, JITUINT32 *memBytesAllocated);
    /** Deserialize Symbol from a stream	*/
    ir_symbol_t * (*deserialize)(void *mem, JITUINT32 memBytes);
    /** Dump the IR symbol in human readable format	*/
    void (*dump)(struct _ir_symbol_t *symbol, FILE *fileToWrite);
} IRSymbolType;

/**
 * \ingroup IRMETHOD
 * @brief An IR item is a parameter to an instruction that can be a variable, constant or symbol.
 */
typedef struct {
    ir_value_t 	value;			/**< Value of the IR constant, ID of the IR variable or ID of the IR symbol.									*/
    JITINT16 	type;                   /**< Type of the parameter. If the parameter is a variable, then this field is equal to IROFFSET.                           			*/
    JITUINT16 	internal_type;          /**< Internal type of the parameter. This field always identifies the type of the parameter and cannot be equal to IROFFSET.			*/
    TypeDescriptor  *type_infos;            /**< If the parameter is a value type, this field points to its CIL type description.								*/
} ir_item_t;

typedef struct {
    JITNUINT        *kill;
    JITNUINT        *gen;
    JITNUINT        *in;
    JITNUINT        *out;
} single_data_flow_t;

typedef struct {
    JITUINT32 num;
    JITUINT32 elements;
    single_data_flow_t      *data;
} data_flow_t;

/**
 * \ingroup IRMETHOD
 * @brief Metadata of an instruction.
 */
typedef struct {
    IrMetadata *metadata;
    struct {
        XanHashTable *alias;            /**< Memory aliases information			*/
    } alias;
    struct {
        IR_ITEM_VALUE def;
        JITNUINT        *use;
        JITNUINT        *in;
        JITNUINT        *out;
    } liveness;
    struct {
        JITNUINT        *kill;
        JITNUINT        *gen;
        JITNUINT        *in;
        JITNUINT        *out;
    } reaching_definitions;
    struct {
        JITNUINT        *kill;
        JITNUINT        *gen;
        JITNUINT        *in;
        JITNUINT        *out;
    } reaching_expressions;
    struct {
        JITNUINT        *kill;
        JITNUINT        *gen;
        JITNUINT        *in;
        JITNUINT        *out;
    } available_expressions;
    struct {
        JITNUINT        *kill;
        JITNUINT        *gen;
        JITNUINT        *in;
        JITNUINT        *out;
    } anticipated_expressions;
    struct {
        JITNUINT        *kill;
        JITNUINT        *use;
        JITNUINT        *in;
        JITNUINT        *out;
    } postponable_expressions;
    struct {
        JITNUINT        *kill;
        JITNUINT        *use;
        JITNUINT        *in;
        JITNUINT        *out;
    } used_expressions;
    struct {
        JITNUINT        *earliestSet;
    } earliest_expressions;
    struct {
        JITNUINT        *latestSet;
    } latest_placements;
    struct {
        IR_ITEM_VALUE newVar;
        JITBOOLEAN analized;
        JITNUINT        *addedInstructions;
    } partial_redundancy_elimination;
    struct {
        XanHashTable 	*dependingInsts; /**< List of instructions depending from this one. Each element is of type (data_dep_arc_list_t *)              */
        XanHashTable 	*dependsFrom;    /**< List of instructions from which this one depends. Each element is of type (data_dep_arc_list_t *)          */
    } data_dependencies;
    XanList         *controlDependencies;   /**< List of type (ir_instruction_t *). The current IR instruction is control dependent on each element of the list.	*/
} ir_instruction_metadata_t;

/**
 * \ingroup IRMETHOD
 * @brief An IR instruction.
 */
typedef struct ir_instruction_t {
    JITUINT32 	ID;                                   /**< ID of the instruction.                              */
    JITUINT16 	type;                                 /**< Type of the instruction.                            */
    ir_item_t 	param_1;                              /**< First parameter to the instruction.                 */
    ir_item_t 	param_2;                              /**< Second parameter to the instruction.                */
    ir_item_t 	param_3;                              /**< Third parameter to the instruction.                 */
    ir_item_t 	result;                               /**< The place that stores the result of the instruction. */
    XanList 	*callParameters;                      /**< List of call parameters for the IR instruction, if it is a call. */
    JITFLOAT32 	executionProbability;                 /**< Probability of executing this instruction if the method is called.	*/
    JITUINT32 	byte_offset;                          /**< Number of the CIL instruction inside the method that binds to this IR instruction.                        */
    JITUINT32	flags;
    ir_instruction_metadata_t       *metadata;            /**< Metadata information for the instruction.	*/
} ir_instruction_t;

/**
 * \ingroup IRMETHOD
 * @brief Information aboutr each circuit within the IR method.
 *
 */
typedef struct {
    JITUINT32 loop_id;                      /**< Circuit identifier.                           */
    JITUINT32 header_id;                    /**< Identifier of the circuit header instruction.             */
    JITNUINT     *belong_inst;              /**< A bitmap of instructions that belong to the circuit.                  */
    JITNUINT     *invariants;               /**< A bitmap of circuit invariant instructions.        */
    JITNUINT     *induction_bitmap;         /**< A bitmap of circuit induction variables.           */
    XanHashTable *induction_table;          /**< A hash table of \c induction_variable_t elements. The key into the hash table for *
                                                 *  an induction variable 'v' is the identifier of 'v', which is a variable ID. Each   *
                                                 *  hash table element is an (induction_variable_t *).                                 *
	                                         *  Due to overflow issues, some induction variables may not have an entry in this     *
	                                         *  table. See Appel A. W. "Modern Compiler Implementation in Java", pag. 388,         *
	                                         *  section "Derived Induction Variables".                                             */
    XanList         *loopExits;             /**< A list of (ir_instruction_t *) that are exits from the circuit.			*/
    struct {
        ir_instruction_t        *src;	/**< Source of the backedge.		*/
        ir_instruction_t        *dst;	/**< Destination of the backedge.	*/
    } backEdge;                            	/**< The back edge for the circuit. 	*/
} circuit_t;

/**
 * \ingroup IRMETHOD
 * @brief A data dependence edge in the Data Dependence Graph (DDG).
 */
typedef struct {
    ir_instruction_t        *inst;                  /**< An instruction that is part of the dependence.								                                */
    JITUINT16 		        depType;                /**< Type of dependence.                                                                                        */
    XanList                 *outsideInstructions;   /**< If the dependence is across methods, this list includes instructions that belong to the caller method,     *
	                                                  *  which are the cause of this dependence.									                                */
} data_dep_arc_list_t;

/**
 * \ingroup IRMETHOD
 * @brief The signature of a method.
 */
typedef struct {
    JITUINT32	parametersNumber;		/**< Number of parameters into the method.							*/
    JITUINT32	resultType;			/**< The result type of the method.								*/
    JITUINT32	*parameterTypes;		/**< The type of each method parameter. Each element in the					*
	                                                  *  array is a JITUINT32 which describes the internal_type of					*
	                                                  *  the parameter										*/
    TypeDescriptor	*ilResultType;			/**< The CIL type description of the result.							*/
    TypeDescriptor	**ilParamsTypes;		/**< Input language parameter types descriptor							*/
    ParamDescriptor **ilParamsDescriptor;		/**< Input language parameters descriptor							*/
} ir_signature_t;

JITUINT32 IRMETHOD_hashIRSignature (ir_signature_t *signature);
JITINT32 IRMETHOD_equalsIRSignature (ir_signature_t *key1, ir_signature_t *key2);
JITINT8 * IRMETHOD_IRSignatureInString (ir_signature_t *signature);

/**
 * \ingroup IRMETHOD
 * @brief A local variable within a method.
 */
typedef struct {
    JITUINT32 var_number;           /**< Number of the variable assigned to the local. */
    JITUINT32 internal_type;        /**< Result type of the method.                          */
    TypeDescriptor  *type_info;     /**< The CIL type description of the local.	*/
} ir_local_t;

/**
 * \ingroup IRMETHOD
 * @brief Information about the IR representation of a method.
 *
 */
typedef struct ir_method_t {
    MethodDescriptor        *ID;
    XanList                 *locals;                /**< CIL type for each local in the method.             */
    pthread_mutex_t		mutex;                  /**< Mutex to protect the method.			*/
    ir_signature_t		signature;              /**< Signature of the method.                           */
    JITUINT32		var_max;                /**< Maximum variables needed by the method.            */
    JITINT8                 *name;                  /**< Name of the method.				*/
    JITUINT64		valid_optimization;
    JITBOOLEAN		modified;               /**< Whether the method has been modified.              */
    JITBOOLEAN		isExternalLinked;
    JITBOOLEAN		isDirectlyReferenced;
    IrMetadata		*metadata;              /**< Metadata for the method.                           */

    /* IR Instructions.				*/
    ir_instruction_t        **instructions;         /**< Instructions that make up the method.              */
    ir_instruction_t        *exitNode;              /**< The last instruction in the method.		*/
    JITUINT32 		instructionsAllocated; 	/**< Number of instruction slots allocated.		*/
    JITUINT32 		instructionsTop;        /**< Number of the first free slot.			*/

    /* Basic block information.			*/
    IRBasicBlock		*basicBlocks;           /**< The basic blocks that make up the method.          */
    JITUINT32		*instructionsBasicBlocksMap;	/**< Mapping from instruction positions to basic block positions	*/
    JITUINT32		basicBlocksTop;         /**< The number of basic blocks in the method.          */

    /* Instruction dominance information.		*/
    XanNode                 *preDominatorTree;      /**< The pre-dominator tree for the IR instructions in the method.		*/
    XanNode                 *postDominatorTree;     /**< The post-dominator tree for the IR instructions in the method.		*/

    /* Loops information.				*/
    XanList                 *loop;                  /**< Circuits within the method. Each element is a (circuit_t *). */
    XanNode                 *loopNestTree;          /**< A nesting tree for circuits within the method. Each element is a (circuit_t*).   */

    /* Data-flow information.			*/
    JITUINT32 livenessBlockNumbers;                 /**< Number of blocks required for liveness information. */
    JITUINT32 reachingDefinitionsBlockNumbers;      /**< Number of blocks required for reaching definitions information. */
    JITUINT32 reachingExpressionsBlockNumbers;      /**< Number of blocks required for reaching expressions information. */
    JITUINT32 availableExpressionsBlockNumbers;     /**< Number of blocks required for available expressions information. */
    JITUINT32 anticipatedExpressionsBlockNumbers;   /**< Number of blocks required for anticipated expressions information. */
    JITUINT32 postponableExpressionsBlockNumbers;   /**< Number of blocks required for postponable expressions information. */
    JITUINT32 usedExpressionsBlockNumbers;          /**< Number of blocks required for used expressions information. */
    JITUINT32 earliestExpressionsBlockNumbers;      /**< Number of blocks required for earliest expressions information. */
    JITUINT32 latestPlacementsBlockNumbers;         /**< Number of blocks required for last placement information. */
    JITUINT32 partialRedundancyEliminationBlockNumbers; /**< Number of blocks required for partial redundancy elimination information. */

    /* Escape variables information.			*/
    struct {
        JITNUINT        *bitmap;
        JITUINT32 	blocksNumber;
    } escapesVariables;
} ir_method_t;

/**
 * \ingroup IRMETHOD_Loop
 * @brief A program loop
 */
typedef struct {
    ir_method_t             *method;                /**< Method that includes the loop								*/
    ir_instruction_t        *header;                /**< Header of the loop										*/
    XanList			*backedges;		/**< Backedges of the loop.									*/
    XanList                 *subLoops;              /**< List of (loop_t *)										*/
    XanList                 *instructions;          /**< List of (ir_instruction_t *). They are the set of instructions that compose the loop	*/
    XanList                 *exits;                 /**< List of (ir_instruction_t *). They are the exit points of the loop				*/
    XanList                 *invariants;            /**< List of (ir_instruction_t *)								*/
    XanList                 *inductionVariables;    /**< List of (induction_variable_t *)								*/
} loop_t;

typedef enum {
    jitScheme,
    dlaScheme,
    staticScheme,
    aotScheme
} compilation_scheme_t;

/**
 * \defgroup IRLIB IR Library
 * \ingroup IRMETHOD
 */
typedef struct ir_lib_t {
    compilation_scheme_t	compilation;
    XanHashTable    	*serialMap;
    JITUINT32 		maxSymbol;
    IRSymbolType    	*deserialMap;
    JITUINT32 		deserialCount;
    XanHashTable		**staticMemorySymbolTable;
    XanHashTable		*loadedSymbols;
    XanHashTable		*globals;
    XanHashTable		*cachedSerialization;
    XanList 		**ilBinaries;

    ir_method_t *   (*newIRMethod)(JITINT8 *name);
    void 		(*createTrampoline)(ir_method_t *method);
    ir_method_t *   (*getIRMethod)(ir_method_t * method, IR_ITEM_VALUE irMethodID, JITBOOLEAN ensureIRTranslation);
    JITINT8 *       (*getProgramName)(void);                                                /**< This function returns the name of the running CIL program.	*/
    void 		(*translateToMachineCode)(ir_method_t * method);
    JITUINT32 	(*getTypeSize)(TypeDescriptor *typeDescriptor);
    ir_method_t *   (*getEntryPoint)(void);
    ir_method_t *   (*getIRMethodFromEntryPoint)(void *entryPointAddress);
    JITINT32 	(*run)(ir_method_t * method, void **args, void *returnArea);
    IR_ITEM_VALUE 	(*getIRMethodID)(ir_method_t * method);
    XanList *       (*getIRMethods)(void);
    XanList *       (*getImplementationsOfMethod)(IR_ITEM_VALUE irMethodID);
    XanList *       (*getCompatibleMethods)(ir_signature_t * signature);
    void *          (*getFunctionPointer)(ir_method_t * method);
    ir_symbol_t *   (*loadSymbol)(JITUINT32 number);
    void 		(*addToSymbolToSave)(ir_symbol_t *symbol);
    ir_method_t *   (*getCallerOfMethodInExecution)(ir_method_t *m);
    void 		(*saveProgram)(JITINT8 *name);
    JITBOOLEAN 	(*hasIRBody)(ir_method_t *method);
    JITBOOLEAN 	(*unregisterMethod)(ir_method_t *method);
    void 		(*tagMethodAsProfiled)(ir_method_t *method);
    JITINT8 * 	(*getNativeMethodName) (JITINT8 *internalName);
} ir_lib_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup IRSYMBOLS_Create
 * @brief Create a new symbol
 *
 * Create a new symbol defined as the instance @c data with the tag @c tag.
 *
 * In the system there can be only one pair (@c tag, @c data).
 * Therefore if this pair has been already created in the system, calling this function does not create a new one, but it returns the already existing one.
 *
 * For example, the assertions in the following code are valid:
 * @code
 * my_symbol_t *mySymbol1;
 * ir_symbol_t *irSymbol1;
 * ir_symbol_t *irSymbol2;
 * JITUINT32 mySymbolTag;
 *
 * // Fetch a free tag
 * mySymbolTag = IRSYMBOL_getFreeSymbolTag();
 *
 * // Allocate my symbol
 * mySymbol1 = allocFunction(sizeof(my_symbol_t));
 *
 * // Set my symbol
 * // ...
 *
 * // Create the correspondent IR symbol
 * irSymbol1 = IRSYMBOL_createSymbol(mySymbolTag, mySymbol1);
 * assert(irSymbol1 != NULL);
 *
 * // Fetch the IR symbol
 * irSymbol2 = IRSYMBOL_createSymbol(mySymbolTag, mySymbol1);
 * assert(irSymbol2 == irSymbol1);
 * @endcode
 *
 * @param tag The type of the returned symbol
 * @param data The identifier of the returned symbol
 * @return The symbol (@c tag, @c data)
 */
ir_symbol_t * IRSYMBOL_createSymbol (JITUINT32 tag, void *data);

/**
 * \ingroup IRSYMBOLS_Resolve
 * @brief Translate a symbol to its actual value
 *
 * Resolve the symbol given as input (i.e.,  @c symbol).
 *
 * Resolving a symbol is done by generating its actual value in the current running instance of the ILDJIT system.
 *
 * @param symbol IR symbol to resolve
 * @return The symbol's actual value
 */
ir_value_t IRSYMBOL_resolveSymbol (ir_symbol_t *symbol);

/**
 * \ingroup IRSYMBOLS_Resolve
 * @brief Translate a symbol to its registered value
 *
 * Translate a symbol to its registered value (i.e., the one given as input at the function \ref IRSYMBOL_createSymbol ).
 *
 * For example, the following code will properly execute without aborting for the included assertion:
 * @code
 *
 * // Create my data
 * MyData_t *myData = ...
 *
 * // Register a symbol
 * ir_symbol_t *sym = IRSYMBOL_createSymbol(myTag, myData);
 *
 * // Retrieve the original value
 * MyData_t *myData2 = IRSYMBOL_unresolveSymbol(sym);
 * assert(myData2 == myData);
 *
 * @endcode
 *
 * @param symbol IR symbol to resolve
 * @return Value registered
 */
void * IRSYMBOL_unresolveSymbol (ir_symbol_t *symbol);

/**
 * \ingroup IRSYMBOLS_Resolve
 * @brief Translate a symbol to its registered value
 *
 * If @c item includes a symbol, then it translates this symbol to its registered value (i.e., the one given as input at the function \ref IRSYMBOL_createSymbol ).
 * In this case, this function returns the same value as the one returned by \ref IRSYMBOL_unresolveSymbol .
 *
 * Otherwise, if @c item does not include a symbol, then it returns NULL.
 *
 * @param item IR symbol to consider
 * @return Value registered
 */
void * IRSYMBOL_unresolveSymbolFromIRItem (ir_item_t *item);

/**
 * \ingroup IRSYMBOLS_Resolve
 * @brief Translate a symbol to its actual value
 *
 * Resolve a symbol within an IR item into another IR item.
 *
 * @param item IR item containing the symbol to resolve
 * @param resolvedSymbol IR item that the symbol will be resolved into
 */
void IRSYMBOL_resolveSymbolFromIRItem (ir_item_t *item, ir_item_t *resolvedSymbol);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief Store a symbol to an IR item
 *
 * Store the symbol @c symbol in the IR item @c item .
 *
 * The symbol must be the output of the call
 * @code
 * IRSYMBOL_createSymbol
 * @endcode
 *
 * For example,
 * @code
 * ir_symbol_t 	*s;
 * ir_item_t 	*irItem;
 *
 * // Fetch an IR item to use to store the symbol
 * irItem = IRMETHOD_getInstructionParameter1(inst);
 *
 * // Fetch the symbolto store
 * s = IRSYMBOL_createSymbol(myTag, mySymbolIdentifier);
 *
 * // Store the symbol
 * IRSYMBOL_storeSymbolToIRItem(irItem, s, IRUINT32, NULL);
 * @endcode
 *
 * @param item IR item that will contain the symbol @c symbol
 * @param symbol Symbol to store
 * @param internal_type IR type of the symbol @c symbol.
 * @param typeInfos Optional high level descriptor of the symbol @c symbol.
 */
void IRSYMBOL_storeSymbolToIRItem (ir_item_t *item, ir_symbol_t *symbol, JITUINT16 internal_type, TypeDescriptor *typeInfos);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief Store a symbol to a parameter of an instruction.
 *
 * Store the symbol @c symbol to the specified parameter of the instruction @c inst.
 *
 * Parameter zero represents the return parameter.
 *
 * The symbol must be the output of the call
 * @code
 * IRSYMBOL_createSymbol
 * @endcode
 *
 * For example,
 * @code
 * ir_symbol_t 	*s;
 * ir_item_t 	*irItem;
 *
 * // Fetch the symbolto store
 * s = IRSYMBOL_createSymbol(myTag, mySymbolIdentifier);
 *
 * // Store the symbol
 * IRSYMBOL_storeSymbolToInstructionParameter(m, inst, s, IRUINT32, NULL);
 * @endcode
 *
 * @param method Method that contains the parameter @c inst.
 * @param inst Instruction where the symbol will be stored to.
 * @param toParam Parameter of @c inst where the symbol will be stored to.
 * @param symbol Symbol to store
 * @param internal_type IR type of the symbol @c symbol.
 * @param typeInfos Optional high level descriptor of the symbol @c symbol.
 */
void IRSYMBOL_storeSymbolToInstructionParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT8 toParam, ir_symbol_t *symbol, JITUINT16 internal_type, TypeDescriptor *typeInfos);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief Symbol table for static memories.
 *
 * Fetch the symbol table for static memories.
 *
 * The returned table has elements where each one represents a unique static memory.
 *
 * Each element has the pointer to the symbol (i.e., ir_symbol *) as key, and the descriptor of the static memory as element (i.e., ir_static_memory_t *);
 *
 * The returned pointer points to the actual global symbol table. Hence, any further updates to it will be seen by the caller of this function as well.
 *
 * @return Pointer to the symbol table that includes all static memories.
 */
XanHashTable * IRSYMBOL_getSymbolTableForStaticMemories (void);

/**
 * \ingroup IRMETHOD_Dump
 * @brief Dump an IR symbol in human readable format to a file.
 *
 * Write the given IR symbol as a string to the given file.
 *
 * @param symbol IR symbol to dump
 * @param fileToWrite File to use to write the IR type
 */
void IRMETHOD_dumpSymbol (ir_symbol_t *symbol, FILE *fileToWrite);

/**
 * \ingroup IRSYMBOLS_IO
 * @brief Cache the serialization of a symbol.
 *
 * Cache the serialization of symbol @c sym.
 *
 * All next invocations of \ref IRSYMBOL_deserializeSymbol will return @c mem and @c memBytes .
 *
 * The memory pointed by @c mem is cloned by this function.
 *
 * @param symbolID Identifier of the symbol to cache
 * @param tag Tag of the symbol to identifier
 * @param mem A pointer to the memory allocated by the caller to store the serialization of the symbol
 * @param memBytes Number of bytes of the memory pointed by @c mem.
 */
void IRSYMBOL_cacheSymbolSerialization (JITUINT32 symbolID, JITUINT32 tag, void *mem, JITUINT32 memBytes);

/**
 * \ingroup IRSYMBOLS_IO
 * @brief Check whether the serialization of a symbol has been cached or not.
 *
 * Return JITTRUE if the serialization of the symbol with identifier @c symbolID has been cached by a previous invocation of \ref IRSYMBOL_cacheSymbolSerialization .
 *
 * Otherwise, JITFALSE is returned.
 *
 * @param symbolID Identifier of the symbol to cache
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRSYMBOL_isSerializationOfSymbolCached (JITUINT32 symbolID);

/**
 * \ingroup IRSYMBOLS_IO
 * @brief Return the serialization of a symbol.
 *
 * If the serialization of symbol @c sym has been previously cached by an invocation of \ref IRSYMBOL_cacheSymbolSerialization , then it stores the information about the serialization of symbol @c sym.
 *
 * @param symbolID Identifier of the symbol to consider
 * @param tag Pointer of the memory location where tag of @c symbolID will be stored into.
 * @param mem New memory initialized with the serialization of @c symbolID will be allocated and its pointer will be stored in this parameter.
 * @param memBytes Pointer of the memory location where the number of bytes of @c mem will be stored into.
 */
void IRSYMBOL_getSerializationOfSymbolCached (JITUINT32 symbolID, JITUINT32 *tag, void **mem, JITUINT32 *memBytes);

/**
 * \ingroup IRSYMBOLS_IO
 * @brief Serialize an IR symbol into a stream. The stream can be deserialized with \c IRMETHOD_deserializeSymbol.
 *
 * Convert the IR symbol into a format that can be stored and loaded later.
 *
 * @param symbol IR symbol to serialize
 * @param mem A pointer to the memory allocated by the caller to store the serialization of the symbol
 * @param memBytesAllocated Number of bytes of the memory pointed by @c mem after the call.
 */
void IRSYMBOL_serializeSymbol (ir_symbol_t *symbol, void **mem, JITUINT32 *memBytesAllocated);

/**
 * \ingroup IRSYMBOLS_IO
 * @brief Deserialize an IR symbol from a stream. The stream can be created with \c IRMETHOD_serializeSymbol
 *
 * Load an IR symbol from a serialized stream in the format used by IRMETHOD_serializeSymbol.
 *
 * @param tag Tag of the symbol to deserialize
 * @param mem Memory where the symbol has been serialized
 * @param memBytes Size of @c mem
 * @return The deserialized symbol
 */
ir_symbol_t * IRSYMBOL_deserializeSymbol (JITUINT32 tag, void *mem, JITUINT32 memBytes);

/**
 * \ingroup IRSYMBOLS_register
 * @brief Register functions for a symbol tag.
 *
 * Register the set of functions given as input as the ones to use to manage all symbols of type @c tag.
 *
 * This function returns JITTRUE if the registration succeeded; JITFALSE otherwise.
 *
 * A registration can fail if previously someone has been already registered to resolve all symbols of type @c tag.
 *
 * @param tag Tag of the symbol to register functions for
 * @param resolve Helper function that resolves the symbol to an actual value
 * @param serialize Helper function that converts the symbol into a format that can be stored
 * @param dump Helper function that writes the symbol in a human-readable format
 * @param deserialize Helper function that restores the symbol from a stored format
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRSYMBOL_registerSymbolManager (JITUINT32 tag, ir_value_t (*resolve)(ir_symbol_t *symbol), void (*serialize)(ir_symbol_t *sym, void **mem, JITUINT32 *memBytesAllocated), void (*dump)(ir_symbol_t *symbol, FILE *fileToWrite), ir_symbol_t * (*deserialize)(void *mem, JITUINT32 memBytes));

/**
 * \ingroup IRSYMBOLS_register
 * @brief Check whether a symbol has been registered or not
 *
 * Check whether a symbol has been registered or not previously.
 *
 * @param tag Tag of the symbol to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRSYMBOL_isSymbolRegistered (JITUINT32 tag);

/**
 * \ingroup IRSYMBOLS_register
 * @brief Get a free symbol tag
 *
 * Return a tag that is not in use at the time this function is called.
 *
 * The returned tag can be used by other functions to register a new symbol tag.
 * @code
 * JITUINT32 freeTag;
 * freeTag = IRSYMBOL_getFreeSymbolTag();
 * assert(!IRSYMBOL_isSymbolRegistered(freeTag));
 * @endcode
 *
 * @return A new tag that can be used for symbols
 */
JITUINT32 IRSYMBOL_getFreeSymbolTag (void);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief From a symbol identifier to an IR symbol
 *
 * Obtain the IR symbol corresponding to the given unique identifier
 *
 * @param number Unique identifier of the symbol to load
 * @return A valid IR symbol
 */
ir_symbol_t * IRSYMBOL_loadSymbol (JITUINT32 number);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief Get all symbols loaded
 *
 * Return the set of symbols loaded till the time of the call.
 *
 * Each key is the symbol identifier (JITUINT32) plus 1.
 *
 * Each element is (ir_symbol_t *).
 *
 * The returned pointer is an alias to the internal table. Hence, subsequent loads of new symbols will be find in the table pointed by the returned pointer.
 *
 * Finally, the caller cannot destroy the returned table.
 *
 * @return Set of symbols loaded
 */
XanHashTable * IRSYMBOL_getLoadedSymbols (void);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief Get the symbol stored inside an IR item
 *
 * If @c item is a symbol (i.e., \ref IRDATA_isASymbol), then this function returns the actual symbol stored.
 * Otherwise, this function returns NULL.
 *
 * @param item IR item to consider
 * @return NULL or the symbol stored in @c item
 */
ir_symbol_t * IRSYMBOL_getSymbol (ir_item_t *item);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief From an IR symbol to a symbol identifier
 *
 * Obtain the symbol identifier from the IR symbol @c symbol .
 *
 * @param symbol Valid IR symbol
 * @return Unique identifier of @c symbol
 */
JITUINT32 IRSYMBOL_getSymbolIdentifier (ir_symbol_t *symbol);

/**
 * \ingroup IRSYMBOLS_Use
 * @brief Check whether a symbol has an identifier.
 *
 * Check whether a symbol has an identifier.
 *
 * @param symbol Valid IR symbol.
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRSYMBOL_hasSymbolIdentifier (ir_symbol_t *symbol);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Initialize a IR Method
 *
 * Initialize a IR Method.
 *
 * @param method Method to initialize
 * @param name Name of the method to initialize
 */
void IRMETHOD_init (ir_method_t *method, JITINT8 *name);

/**
 * \ingroup IRLIB
 * @brief Destroy an IR library.
 *
 * @param irLib IR library to destroy
 */
void IRLibDestroy (ir_lib_t *irLib);

/**
 * \ingroup IRLIB
 * @brief Create and initialize a new IR library.
 */
void IRLibNew(
    ir_lib_t * ir_lib,
    compilation_scheme_t compilation,
    ir_method_t *   (newIRMethod) (JITINT8 *name),
    void(createTrampoline) (ir_method_t * method),
    ir_method_t *   (*getIRMethod)(ir_method_t * method, IR_ITEM_VALUE irMethodID, JITBOOLEAN ensureIRTranslation),
    JITINT8 *       (*getProgramName)(void),
    void (*translateToMachineCode)(ir_method_t * method),
    JITINT32 (*run)(ir_method_t * method, void **args, void *returnArea),
    JITUINT32 (*getTypeSize)(TypeDescriptor *typeDescriptor),
    ir_method_t *   (*getEntryPoint)(void),
    ir_method_t *   (*getIRMethodFromEntryPoint)(void *entryPointAddress),
    IR_ITEM_VALUE (*getIRMethodID)(ir_method_t * method),
    XanList *       (*getIRMethods)(void),
    XanList *       (*getImplementationsOfMethod)(IR_ITEM_VALUE irMethodID),
    XanList *       (*getCompatibleMethods)(ir_signature_t * signature),
    void *          (*getFunctionPointer)(ir_method_t * method),
    ir_symbol_t *   (*loadSymbol)(JITUINT32 number),
    void (*addToSymbolToSave)(ir_symbol_t *symbol),
    ir_method_t *   (*getCallerOfMethodInExecution)(ir_method_t *m),
    void (*saveProgram)(JITINT8 *name),
    JITBOOLEAN (*hasIRBody)(ir_method_t *method),
    JITBOOLEAN (*unregisterMethod)(ir_method_t *method),
    void (*tagMethodAsProfiled)(ir_method_t *method),
    JITINT8 * (*getNativeMethodName) (JITINT8 *internalName),
    XanHashTable **staticMemorySymbolTable,
    XanList **ilBinaries
);

/**
 * \ingroup IRLIB
 * @brief Return the version of the libiljitir library as a string.
 *
 * @return Version of the library
 */
char * libirmanagerVersion ();

/**
 * \ingroup IRLIB
 * @brief Write the compilation flags used for the libiljitir library to a buffer.
 *
 * @param buffer A buffer to write to
 * @param bufferLength length of the buffer to write to
 */
void libirmanagerCompilationFlags (char *buffer, JITINT32 bufferLength);

/**
 * \ingroup IRLIB
 * @brief Write the time that the libiljitir library was compiled to a buffer.
 *
 * @param buffer A buffer to write to
 * @param bufferLength length of the buffer to write to
 */
void libirmanagerCompilationTime (char *buffer, JITINT32 bufferLength);


/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the available expressions generated by an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get available expressions generated
 * @param blockNumber Block number of available expressions to get
 * @return A bitmap of available expressions generated for this instruction in this block
 */
JITNUINT ir_instr_available_expressions_gen_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the available expressions generated by an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set available expressions
 * @param blockNumber Block number of available expressions to set
 * @param value Value the block should be set to
 */
void ir_instr_available_expressions_gen_set (void *method, ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the available expressions out of an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get available expressions out
 * @param blockNumber Block number of available expressions to get
 * @return A bitmap of available expressions out of this instruction in this block
 */
JITNUINT ir_instr_available_expressions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the available expressions out of an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set available expressions
 * @param blockNumber Block number of available expressions to set
 * @param value Value the block should be set to
 */
void ir_instr_available_expressions_out_set (ir_method_t *method, ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the available expressions into an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get available expressions in
 * @param blockNumber Block number of available expressions to get
 * @return A bitmap of available expressions into this instruction in this block
 */
JITNUINT ir_instr_available_expressions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the available expressions into an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set available expressions
 * @param blockNumber Block number of available expressions to set
 * @param value Value the block should be set to
 */
void ir_instr_available_expressions_in_set (void *method, ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the available expressions killed by an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get available expressions that are killed
 * @param blockNumber Block number of available expressions to get
 * @return A bitmap of available expressions killed by this instruction in this block
 */
JITNUINT ir_instr_available_expressions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the available expressions killed by an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set available expressions
 * @param blockNumber Block number of available expressions to set
 * @param value Value the block should be set to
 */
void ir_instr_available_expressions_kill_set (void *method, ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Determine whether an expression is available out of an instruction.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to determine whether the expression is available
 * @param expressionID Expression to determine availability for
 * @return JITTRUE if the expression is available, JITFALSE otherwise
 */
JITBOOLEAN ir_instr_available_expressions_available_out_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Determine whether an expression is available into an instruction.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to determine whether the expression is available
 * @param expressionID Expression to determine availability for
 * @return JITTRUE if the expression is available, JITFALSE otherwise
 */
JITBOOLEAN ir_instr_available_expressions_available_in_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID);


/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the variables live out of an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get the variables that are live
 * @param blockNumber Liveness block number to get
 * @return A bitmap of variables live out of this instruction in this block
 */
JITNUINT ir_instr_liveness_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the variables live out of an instruction within a method's liveness information.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set the variables that are live out
 * @param blockNumber Liveness block number to set
 * @param value Value that this block should be set to
 */
void ir_instr_liveness_out_set (void *method, ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the variables live into an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get the variables that are live
 * @param blockNumber Liveness block number to get
 * @return A bitmap of variables live into this instruction in this block
 */
JITNUINT ir_instr_liveness_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the variables live into an instruction within a method's liveness information.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set the variables that are live in
 * @param blockNumber Liveness block number to set
 * @param value Value that this block should be set to
 */
void ir_instr_liveness_in_set (void *method, ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the variables used by an instruction in a specific block.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get the variables that are live
 * @param blockNumber Liveness block number to get
 * @return A bitmap of variables live used by this instruction in this block
 */
JITNUINT ir_instr_liveness_use_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the variables used by an instruction within a method's liveness information.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set the variables that are used
 * @param blockNumber Liveness block number to set
 * @param value Value that this block should be set to
 */
void ir_instr_liveness_use_set (void *method, ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Get the variable defined by an instruction.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to get the variable that is defined
 * @return Variable defined by this instruction
 */
JITNUINT ir_instr_liveness_def_get (void *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set the variable defined by an instruction within a method's liveness information.
 *
 * @param method IR method that the instruction is in
 * @param inst Instruction for which to set the variable that is defined
 * @param value Variable defined by this instruction
 */
void ir_instr_liveness_def_set (void *method, ir_instruction_t *inst, JITNUINT value);


/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Check whether a variable is live before an instruction.
 *
 * Check whether the variable <code> varID </code> belongs to the set IN of the instruction <code> inst </code>.
 *
 * @param method IR method where both the instructions belong to
 * @param inst IR instruction to consider
 * @param varID Variable ID to consider
 * @return JITTTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isVariableLiveIN (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Check if a variable is live after an instruction
 *
 * Check if the variable <code> varID </code> belongs to the set OUT of the instruction <code> inst </code>.
 *
 * @param method IR method where both the instructions belong to
 * @param inst IR instruction to consider
 * @param varID Variable ID to consider
 * @return JITTTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isVariableLiveOUT (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID);

#define livenessGetDef(method, instr) ((instr->metadata->liveness).def)
#define livenessGetOut(method, instr, blockNumber) ((instr->metadata->liveness).out[blockNumber])
#define livenessGetIn(method, instr, blockNumber) ((instr->metadata->liveness).in[blockNumber])
#define livenessGetUse(method, instr, blockNumber) ((instr->metadata->liveness).use[blockNumber])
#define livenessSetOut(method, instr, blockNumber, value) ((instr->metadata->liveness).out[blockNumber] = value)
#define livenessSetIn(method, instr, blockNumber, value) ((instr->metadata->liveness).in[blockNumber] = value)
#define livenessSetUse(method, instr, blockNumber, value) ((instr->metadata->liveness).use[blockNumber] = value)
#define livenessSetDef(method, instr, value) ((instr->metadata->liveness).def = value)

/* Reaching definitions			*/
JITNUINT ir_instr_reaching_definitions_gen_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);
void ir_instr_reaching_definitions_gen_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);
JITNUINT ir_instr_reaching_definitions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);
void ir_instr_reaching_definitions_out_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);
JITNUINT ir_instr_reaching_definitions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);
void ir_instr_reaching_definitions_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);
JITNUINT ir_instr_reaching_definitions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber);
void ir_instr_reaching_definitions_kill_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);
JITBOOLEAN ir_instr_reaching_definitions_reached_out_is (void *method, ir_instruction_t *inst, JITUINT32 definitionID);
JITBOOLEAN ir_instr_reaching_definitions_reached_in_is (void *method, ir_instruction_t *inst, JITUINT32 definitionID);
#define reachingDefinitionsGetIn(method, instr, blockNumber) ((instr->metadata->reaching_definitions).in[blockNumber])
#define reachingDefinitionsGetOut(method, instr, blockNumber) ((instr->metadata->reaching_definitions).out[blockNumber])
#define reachingDefinitionsGetGen(method, instr, blockNumber) ((instr->metadata->reaching_definitions).gen[blockNumber])
#define reachingDefinitionsGetKill(method, instr, blockNumber) ((instr->metadata->reaching_definitions).kill[blockNumber])
#define reachingDefinitionsSetIn(method, instr, blockNumber, value) ((instr->metadata->reaching_definitions).in[blockNumber] = value)
#define reachingDefinitionsSetOut(method, instr, blockNumber, value) ((instr->metadata->reaching_definitions).out[blockNumber] = value)
#define reachingDefinitionsSetGen(method, instr, blockNumber, value) ((instr->metadata->reaching_definitions).gen[blockNumber] = value)
#define reachingDefinitionsSetKill(method, instr, blockNumber, value) ((instr->metadata->reaching_definitions).kill[blockNumber] = value)

/**
 * \ingroup IRMETHODMETADATA
 * @brief Get the metadata bound to an IR method
 *
 * Get the metadata previously registered to the method <code> method </code> of type specified as input by <code> type </code>.
 *
 * This function returns NULL if no metadata is found.
 *
 * @param method IR method to consider
 * @param type Type of the returned metadata
 * @return Metadata requested or NULL
 */
void * IRMETHOD_getMethodMetadata (ir_method_t *method, JITUINT64 type);

/**
 * \ingroup IRMETHODMETADATA
 * @brief Bind a metadata to an IR method
 *
 * Bind the metadata <code> data </code> to the method <code> method </code> of type specified as input by <code> type </code>.
 *
 * @param method IR method to consider
 * @param type Type of the returned metadata
 * @param data Data to set as metadata \c type
 * @return data Metadata to bind
 */
void IRMETHOD_setMethodMetadata (ir_method_t *method, JITUINT64 type, void *data);

/**
 * \ingroup IRMETHODMETADATA
 * @brief Remove metadata attached to an IR method
 *
 * Remove all metadata of type <code> type </code> from the method @c method .
 *
 * @param method IR method to consider
 * @param type Type of the removed metadata
 */
void IRMETHOD_removeMethodMetadata (ir_method_t *method, JITUINT64 type);

/**
 * \ingroup IRMETHODMETADATA
 * @brief Get the metadata bound to an IR instruction
 *
 * Get the metadata previously registered to the instruction <code> inst </code> of type specified as input by <code> type </code>.
 *
 * This function returns NULL if no metadata is found.
 *
 * @param inst IR instruction to consider
 * @param type Type of the returned metadata
 * @return Metadata requested or NULL
 */
void * IRMETHOD_getInstructionMetadata (ir_instruction_t *inst, JITUINT64 type);

/**
 * \ingroup IRMETHODMETADATA
 * @brief Bind a metadata to an IR instruction
 *
 * Bind the metadata <code> data </code> to the instruction <code> inst </code> of type specified as input by <code> type </code>.
 *
 * @param inst IR instruction to consider
 * @param type Type of the metadata to set
 * @param data Metadata of type @c type to set for the instruction @c inst
 */
void IRMETHOD_setInstructionMetadata (ir_instruction_t *inst, JITUINT64 type, void *data);

/**
 * \ingroup IRMETHOD_DataDependences
 * @brief Get instructions that an instruction depends from
 *
 * Returns the set of dependences of the instruction @c fromInst.
 *
 * If no dependences exist, it returns NULL.
 *
 * Each element in the list is (data_dep_arc_list_t *).
 * These elements are pointers to internal data structures.
 * Therefore, modifying them will effect the system.
 *
 * The set of instructions included in these elements are the instructions that need to be executed before @c fromInst.
 *
 * The caller is responsible to destroy the returned list as following:
 * @code
 * XanList *deps;
 *
 * // Fetch the dependences
 * deps = IRMETHOD_getDataDependencesFromInstruction(method, fromInst);
 *
 * // Use the dependences
 * // ...
 *
 * // Free the memory
 * xanList_destroyList(deps);
 * @endcode
 *
 * @param method Method that includes @c fromInst
 * @param fromInst Instruction to consider
 * @return Dependences of @c frominst
 */
XanList * IRMETHOD_getDataDependencesFromInstruction (ir_method_t *method, ir_instruction_t *fromInst);

/**
 * \ingroup IRMETHOD_DataDependences
 * @brief Get instructions that depend on a given one
 *
 * Returns the set of instructions that depend on @c toInst.
 *
 * If no dependences exist, it returns NULL.
 *
 * Each element in the list is (data_dep_arc_list_t *).
 * These elements are pointers to internal data structures.
 * Therefore, modifying them will effect the system.
 *
 * The caller is responsible to destroy the returned list as following:
 * @code
 * XanList *deps;
 *
 * // Fetch the dependences
 * deps = IRMETHOD_getDataDependencesToInstruction(method, toInst);
 *
 * // Use the dependences
 * // ...
 *
 * // Free the memory
 * xanList_destroyList(deps);
 * @endcode
 *
 * @param method Method that includes @c toInst
 * @param toInst Instruction to consider
 * @return Dependences of @c toInst
 */
XanList * IRMETHOD_getDataDependencesToInstruction (ir_method_t *method, ir_instruction_t *toInst);

/**
 * \ingroup IRMETHOD_DataDependencesAdd
 * @brief Add a data dependence between instructions
 *
 * Add the data dependence from the IR instruction <code> fromInst </code> to <code> toInst </code>, if it does not exist yet.
 *
 * If the list <code> outsideInstructions </code> is not empty, it is cloned inside; the caller should free it after calling this function.
 *
 * @param method IR method where both the instructions belong to
 * @param fromInst Source of the dependence
 * @param toInst Destination of the dependence
 * @param depType Type of dependence
 * @param outsideInstructions If <code> toInst </code> is a call instruction, then this list is a set of instructions of the called method, which cause the dependence.
 * @return JITTTRUE if the dependence is added; JITFALSE if it already exists
 */
JITBOOLEAN IRMETHOD_addInstructionDataDependence (ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst, JITUINT16 depType, XanList *outsideInstructions);

/**
 * \ingroup IRMETHOD_DataDependencesRemove
 * @brief Delete a specific type of directed data dependence between instructions
 *
 * Delete the directed data dependence of type <code> depType </code> from the IR instruction <code> from </code> to <code> to </code>, if it exists.
 *
 * @param method IR method where both the instructions belong to
 * @param from Source of the dependence
 * @param to Destination of the dependence
 * @param depType Types of the data dependence to remove
 */
void IRMETHOD_removeInstructionDataDependence (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to, JITUINT16 depType);

/**
 * \ingroup IRMETHOD_DataDependencesRemove
 * @brief Delete a directed data dependence between instructions
 *
 * Delete all directed data dependence from the IR instruction <code> from </code> to <code> to </code>, if any exist.
 *
 * @param method IR method where both the instructions belong to
 * @param from Source of the dependence
 * @param to Destination of the dependence
 * @return JITTTRUE if the dependence existed; JITFALSE otherwise.
 */
JITBOOLEAN IRMETHOD_deleteInstructionsDataDependence (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to);

/**
 * \ingroup IRMETHOD_DataDependencesRemove
 * @brief Delete all data dependences related to the instruction.
 *
 * Remove and free the memory used for all data dependences related to the instruction given as input.
 *
 * This function removes memory aliases information as well.
 *
 * @param method Method that includes \c inst
 * @param inst Instruction to consider.
 */
void IRMETHOD_destroyInstructionDataDependences (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_DataDependencesRemove
 * @brief Delete all data dependences specified among instructions within a method
 *
 * Remove and free the memory used for all data dependences specified among instructions within a method.
 *
 * This function destroys memory aliases as well by calling \ref IRMETHOD_destroyMemoryAliasInformation .
 *
 * @param method IR method to consider
 */
void IRMETHOD_destroyDataDependences (ir_method_t *method);

/**
 * \ingroup IRMETHOD_DataDependences
 * @brief Get the data dependence between instructions
 *
 * If the IR instruction <code> from </code> is data dependent on <code> to </code> then return the type of data dependence.
 * Return 0 otherwise.
 *
 * @param method IR method where both the instructions belong to
 * @param from Source of the dependence
 * @param to Destination of the dependence
 * @return The dependence if it exists; 0 otherwise
 */
JITUINT16 IRMETHOD_getInstructionDataDependence (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to);

/**
 * \ingroup IRMETHOD_DataDependences
 * @brief Check a data dependence between instructions
 *
 * Return JITTRUE if the IR instruction <code> from </code> is data dependent on <code> to </code>.
 * JITFALSE otherwise.
 *
 * @param method IR method where both the instructions belong to
 * @param from Source of the dependence
 * @param to Destination of the dependence
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRMETHOD_isInstructionDataDependent (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to);

/**
 * \ingroup IRMETHOD_DataDependences
 * @brief Check a memory data dependence between instructions
 *
 * Return JITTRUE if the IR instruction <code> from </code> is data dependent on <code> to </code> due to memory operations.
 * JITFALSE otherwise.
 *
 * @param method IR method where both the instructions belong to
 * @param from Source of the dependence
 * @param to Destination of the dependence
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRMETHOD_isInstructionDataDependentThroughMemory (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to);

/**
 * \ingroup IRMETHOD_DataDependences
 * @brief Check if a data dependence is through memory
 *
 * Return JITTRUE if the data dependence @c type is a memory one.
 *
 * JITFALSE otherwise.
 *
 * @param depType Type of dependence
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRMETHOD_isAMemoryDataDependenceType (JITUINT16 depType);

/**
 * \ingroup IRMETHOD_ModifyMethodsModifyingInstructions
 * @brief Shift the variable IDs
 *
 * IR instructions use variables which are identified by a progressive number called ID of the variable (e.g. var 5).
 * This function shifts by \c offset_value every variable ID of the IR method \c method given as input.
 * This function performs the same task as the function \c IRMETHOD_shiftVariableIDsHigherThanThreshold excepts that it targets every variable rather than a subset.
 *
 * @param method IR method to consider
 * @param offset_value Delta to add to every variable IDs
 */
void IRMETHOD_shiftVariableIDs (ir_method_t *method, JITINT32 offset_value);

/**
 * \ingroup IRMETHOD_ModifyMethodsModifyingInstructions
 * @brief Shift the variable IDs
 *
 * IR instructions use variables which are identified by a progressive number called ID of the variable (e.g. var 5).
 * Shift by \c offset_value every variable ID strictly grater than \c threshold of the method \c method given as input.
 *
 * @param method IR method to consider
 * @param offset_value Delta to add to every variable IDs
 * @param threshold Threshold that defines the subset of variables to consider
 */
void IRMETHOD_shiftVariableIDsHigherThanThreshold (ir_method_t *method, JITINT32 offset_value, JITINT32 threshold);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_sub
 * @brief Substitute variables of a IR instruction
 *
 * Substitute every reference of variable <code> fromVarID </code> of the instruction given as input with the variable <code> toVarID </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst IR instruction to consider
 * @param fromVarID Variable to substitute
 * @param toVarID New variable to use
 */
void IRMETHOD_substituteVariableInsideInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_sub
 * @brief Substitute variables of a IR instruction with an IR item
 *
 * Substitute every reference of variable <code> fromVarID </code> of the instruction given as input with the IR item <code> item </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst IR instruction to consider
 * @param fromVarID Variable to substitute
 * @param itemToUse IR item to use within the substitution
 */
void IRMETHOD_substituteVariableInsideInstructionWithItem (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, ir_item_t *itemToUse);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_sub
 * @brief Substitute a variable within its uses within an IR instruction
 *
 * Substitute every reference of variable <code> fromVarID </code> of the instruction given as input with the variable <code> toVarID </code> that correspond to a use of <code> fromVarID </code>.
 *
 * The difference with \ref IRMETHOD_substituteVariableInsideInstruction is that this function does not change the variable within the result of <code> inst </code>.
 *
 * For example, consider the following IR instruction:
 * @code
 * var 5 = add (var 5, var 2)
 * @endcode
 * By calling
 * @code
 * IRMETHOD_substituteUsesOfVariableInsideInstruction(method, inst, 5, 10)
 * @endcode
 * the instruction becomes
 * @code
 * var 5 = add (var 10, var 2)
 * @endcode
 * On the other hand, by calling
 * @code
 * IRMETHOD_substituteVariableInsideInstruction(method, inst, 5, 10)
 * @endcode
 * the instruction becomes
 * @code
 * var 10 = add (var 10, var 2)
 * @endcode
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst IR instruction to consider
 * @param fromVarID Variable to substitute
 * @param toVarID New variable to use
 */
void IRMETHOD_substituteUsesOfVariableInsideInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID);

/**
 * \ingroup IRMETHOD_ModifyMethodsModifyingInstructions
 * @brief Substitute variables within a method
 *
 * Substitute every reference of variable <code> fromVarID </code> with the variable <code> toVarID </code> to every instruction of the method given as input.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param fromVarID Variable to substitute
 * @param toVarID New variable to use
 */
void IRMETHOD_substituteVariable (ir_method_t *method, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_sub
 * @brief Substitute a label of an IR instruction
 *
 * Substitute every reference of label <code> old_label </code> of the instruction given as input with the label <code> new_label </code>.
 *
 * Both <code> old_label </code> and <code> new_label </code> are IR label IDs (i.e., <code> IRLABELITEM </code>).
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction to consider
 * @param old_label Label to substitute
 * @param new_label New label to use
 */
void IRMETHOD_substituteLabel (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE old_label, IR_ITEM_VALUE new_label);

/**
 * \ingroup IRMETHOD_ModifyMethodsModifyingInstructions
 * @brief Redirect branches from a label to another one
 *
 * Substitute every reference of label <code> old_label </code> inside every branch instruction with the label <code> new_label </code>.
 *
 * Both <code> old_label </code> and <code> new_label </code> are IR label IDs (i.e., <code> IRLABELITEM </code>).
 *
 * @param method IR method that includes the instruction \c inst
 * @param old_label Label to substitute
 * @param new_label New label to use
 */
void IRMETHOD_substituteLabelInAllBranches (ir_method_t *method, IR_ITEM_VALUE old_label, IR_ITEM_VALUE new_label);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_sub
 * @brief Substitute every labels of an IR instruction
 *
 * Substitute every reference of any label of the instruction given as input with the label <code> new_label </code>.
 *
 * The parameter <code> new_label </code> has to be an IR label ID (i.e., <code> IRLABELITEM </code>).
 *
 * The difference with \ref IRMETHOD_substituteLabel is that this function substitutes any label rather than a specific one.
 *
 * For example, consider the following IR instruction:
 * @code
 * branch_if_pc_not_in_range(1, 2, 3)
 * @endcode
 * 1, 2 and 3 are labels (i.e., <code> IRLABELITEM </code>).\n
 * By calling
 * @code
 * IRMETHOD_substituteLabels(method, inst, 5)
 * @endcode
 * the instruction becomes
 * @code
 * branch_if_pc_not_in_range(5, 5, 5)
 * @endcode
 * On the other hand, by calling
 * @code
 * IRMETHOD_substituteLabel(method, inst, 1, 5)
 * @endcode
 * the instruction becomes
 * @code
 * branch_if_pc_not_in_range(5, 2, 3)
 * @endcode
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst IR instruction to consider
 * @param new_label New label to use
 */
void IRMETHOD_substituteLabels (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE new_label);

/**
 * \ingroup IRMETHOD_Method_VariablesAlias
 * @brief Check if two variables may alias
 *
 * This function checks if variable \c i and variable \c j can be alias at the position in the CFG identified by the instruction \c inst.
 *
 * @param method Method that includes the instruction given as input
 * @param inst Position of the CFG where the alias relation is checked
 * @param i First variable of the alias relation
 * @param j Second variable of the alias relation
 * @return JITTRUE if variable \c i may store the same address of the variable \c j at runtime; JITFALSE otherwise
 */
JITBOOLEAN IRMETHOD_mayAlias (ir_method_t *method, ir_instruction_t *inst, JITUINT32 i, JITUINT32 j);

/**
 * \ingroup IRMETHOD_Method_VariablesAlias
 * @brief Set two variables as aliases
 *
 * Declare the alias between variables \c i and \c j at the instruction \c inst .
 *
 * @param method Method that includes the instruction given as input
 * @param inst Position of the CFG where the alias is declared
 * @param i First variable of the alias relation
 * @param j Second variable of the alias relation
 */
void IRMETHOD_setAlias (ir_method_t *method, ir_instruction_t *inst, JITUINT32 i, JITUINT32 j);

/**
 * \ingroup IRMETHOD_Method_VariablesAlias
 * @brief Destroy memory alias information
 *
 * This function frees the memory used to store the memory alias information.
 *
 * It does not free the memory used to store data dependences. In order to destroy data dependences as well, call \ref IRMETHOD_destroyDataDependences .
 *
 * @param method Method to consider
 */
void IRMETHOD_destroyMemoryAliasInformation (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionDominance
 * @brief Returns the immediate predominator
 *
 * Returns the immediate predominator of the instruction given as input.
 * The caller should ensure that the predominator tree has been computed before the invocation of this function.
 * In order to ensure it, you should call the PRE_DOMINATOR_COMPUTER plugin and you should ensure that after that, no one has modified the Control Flow Graph of the method.
 *
 * @param method Method that includes \c inst
 * @param inst Instruction to consider
 * @return Immediate predominator of \c inst
 */
ir_instruction_t * IRMETHOD_getImmediatePredominator (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionDominance
 * @brief Check if an instruction pre-dominates another one
 *
 * Return JITTRUE if the instruction \c inst pre-dominates the instruction \c dominated given as input.
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @param dominated Instruction to check if it is dominated by \c inst
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isInstructionAPredominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated);

/**
 * \ingroup IRMETHOD_InstructionDominance
 * @brief Check if an instruction pre-dominates a set of instructions
 *
 * Return JITTRUE if the instruction \c inst pre-dominates all instructions included in the list \c dominatedInstructions given as input.
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @param dominatedInstructions Instructions to check. List of (ir_instruction_t *).
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isInstructionAPredominatorOfAllInstructions (ir_method_t *method, ir_instruction_t *inst, XanList *dominatedInstructions);

/**
 * \ingroup IRMETHOD_InstructionDominance
 * @brief Check if an instruction post-dominates another one
 *
 * Return JITTRUE if the instruction \c inst post-dominates the instruction \c dominated given as input.
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @param dominated Instruction to check if it is dominated by \c inst
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isInstructionAPostdominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Number of predecessors of an instruction
 *
 * Returns the number of predecessors of the instruction inst given as input.
 *
 * @param method IR method that includes @c inst.
 * @param inst IR instruction to consider
 * @return Number of instruction predecessors of @c inst.
 */
JITUINT32 IRMETHOD_getPredecessorsNumber (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Determine whether an instruction is a predecessor of another.
 *
 * @param method The method that the instructions belong to
 * @param inst The instruction to check for predecessors
 * @param pred The potential predecessor of <code> inst </code>
 * @return JITTRUE if <code> pred </code> is a predecessor of <code> inst </code>, JITFALSE otherwise
 */
JITBOOLEAN IRMETHOD_isAPredecessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *pred);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Number of successors of an instruction
 *
 * Returns the number of successors of the instruction inst given as input.
 *
 * @param method IR method that includes @c inst.
 * @param inst IR instruction to consider
 * @return Number of instruction successors of @c inst.
 */
JITUINT32 IRMETHOD_getSuccessorsNumber (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Determine whether an instruction is a successor of another.
 *
 * @param method The method that the instructions belong to
 * @param inst The instruction to check for successors
 * @param succ The potential successor of <code> inst </code>
 * @return JITTRUE if <code> succ </code> is a successor of <code> inst </code>, JITFALSE otherwise
 */
JITBOOLEAN IRMETHOD_isASuccessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *succ);

/**
 * \ingroup IRMETHOD_InstructionUses
 * @brief Check if an instruction uses a variable
 *
 * Return JITTRUE if the instruction given as input uses the variable varID; JITFALSE otherwise.
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @param varID IR variable to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doesInstructionUseVariable (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction uses a label
 *
 * Return JITTRUE if the instruction given as input uses the label labelID; JITFALSE otherwise.
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @param labelID Label to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doesInstructionUseLabel (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE labelID);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the number of instructions of a method
 *
 * Return the number of IR instructions that compose the IR method given as input.
 *
 * @param method IR method
 * @return Number of instructions of the method given as input
 */
JITUINT32 IRMETHOD_getInstructionsNumber (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Check if one instruction at least of the input set uses a variable
 *
 * Returns JITTRUE if there is at least an instruction stored inside the list given as input that uses the variable varID; JITFALSE otherwise.
 *
 * @param method IR method where \c inst belongs to
 * @param instructions IR instructions to consider
 * @param varID IR variable to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doInstructionsUseVariable (ir_method_t *method, XanList *instructions, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Check if one instruction at least of the input set defines a variable
 *
 * Returns JITTRUE if there is at least an instruction stored inside the list given as input that defines the variable varID; JITFALSE otherwise.
 *
 * @param method IR method where \c instructions belongs to
 * @param instructions IR instructions to consider
 * @param varID IR variable to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doInstructionsDefineVariable (ir_method_t *method, XanList *instructions, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Check if instructions have the same execution effects
 *
 * Returns JITTRUE if all instructions (considered one at a time) have the same effects when they are executed; JITFALSE otherwise.
 *
 * @param method IR method where \c instructions belongs to
 * @param instructions IR instructions to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_haveInstructionsTheSameExecutionEffects (ir_method_t *method, XanList *instructions);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if two instructions have the same execution effects
 *
 * Returns JITTRUE if both the instructions given as input (considered one at a time) have the same effects when they are executed; JITFALSE otherwise.
 *
 * @param method IR method where \c instructions belongs to
 * @param inst1 IR instruction to consider
 * @param inst2 IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_haveBothInstructionsTheSameExecutionEffects (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2);

/**
 * \ingroup IRMETHOD_InstructionDefine
 * @brief Check if an instruction defines the variable specified in input
 *
 * Returns JITTRUE if the instruction given as input define the variable varID; JITFALSE otherwise.
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @param varID IR variable to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doesInstructionDefineVariable (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_InstructionDefine
 * @brief Check if an instruction defines any variable
 *
 * Returns JITTRUE if the instruction inst may define a variable (any one); JITFALSE otherwise
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doesInstructionDefineAVariable (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionDefine
 * @brief Get the variable defined by an instruction
 *
 * If the instruction inst may define a variable (any one), then this method returns its variable ID.
 * Otherwise, this method returns NOPARAM
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @return NOPARAM or the variable defined by \c inst
 */
IR_ITEM_VALUE IRMETHOD_getVariableDefinedByInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionUses
 * @brief Return the set of variables used by an instruction.
 *
 * Return the set of variables used by an instruction.
 *
 * The returned list is a set of (ir_instruction_t *).
 *
 * The caller is in charge to free the memory of the returned list.
 *
 * An example of use of this function is the following:
 *
 * @code
 * XanList *vars;
 *
 * // Fetch the set of variables used by the instruction inst
 * vars = IRMETHOD_getVariablesUsedByInstruction(inst);
 *
 * // Use the variables
 * item = xanList_first(vars);
 * while (item != NULL){
 * 	ir_item_t *v;
 *	v	= item->data;
 *	assert(IRDATA_isAVariable(v));
 *	printf("Var %llu\n", v->value.v);
 * 	item	= item->next;
 * }
 *
 * // Free the memory
 * xanList_destroyList(vars);
 * @endcode
 *
 * @param inst IR instruction to consider
 * @return List of (ir_item_t *)
 */
XanList * IRMETHOD_getVariablesUsedByInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionDefine
 * @brief Get the IR item that includes the variable defined by an instruction
 *
 * If the instruction inst may define a variable (any one), then this function returns the IR item that includes the variable ID that this instruction defines.
 * Otherwise, this method returns NULL.
 *
 * The returned IR Item is a pointer to the internal structure of the instruction, therefore:
 * <ul>
 * <li> the caller cannot frees it and
 * <li> modifying the returned data structure corresponds to modify the instruction @c inst
 * </ul>
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @return NOPARAM or the variable defined by \c inst
 */
ir_item_t * IRMETHOD_getIRItemOfVariableDefinedByInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Dump
 * @brief Dump the IR type to the file specified in input
 *
 * Write the IR item as a string to the file specified in input.
 *
 * @param method Method that includes the IR item @c item
 * @param item IR variable or constant to dump
 * @param fileToWrite File to use to write the IR type
 */
void IRMETHOD_dumpIRItem (ir_method_t *method, ir_item_t *item, FILE *fileToWrite);

/**
 * \ingroup IRMETHOD_Dump
 * @brief Return the name of the IR type
 *
 * Return the string representation of the IR type given as input.
 *
 * @param type IR type to consider
 * @return String representation of \c type
 */
JITINT8 * IRMETHOD_getIRTypeName (JITUINT32 type);

/**
 * \ingroup IRMETHOD_Dump
 * @brief Dump the IR type to the file specified in input
 *
 * Write the IR type as a string to the file specified in input.
 *
 * @param type IR type to dump
 * @param fileToWrite File to use to write the IR type
 */
void IRMETHOD_dumpIRType (JITUINT32 type, FILE *fileToWrite);

/**
 * \ingroup IRMETHOD_Dump
 * @brief Return the name of an instruction type
 *
 * Return the string form of the instruction type <code> instructionType </code>.
 *
 * @param instructionType IR type to consider
 * @return Name of the instruction type given as input
 */
JITINT8 * IRMETHOD_getInstructionTypeName (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_ControlDependences
 * @brief Returns a bitset representing the set of instructions the given one depends on
 *
 * The bitset will have to be explicitly allocated after use
 *
 * @param method Method that includes @c inst
 * @param inst Instruction to consider
 */
XanBitSet * IRMETHOD_getControlDependencesBitSet (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ControlDependences
 * @brief Add a control dependence between instructions
 *
 * Add the control dependence from the IR instruction <code> fromInst </code> to <code> toInst </code>, if it does not exist yet.
 *
 * @param method IR method where both the instructions belong to
 * @param fromInst Source of the dependence
 * @param toInst Destination of the dependence
 * @return JITTTRUE if the dependence is added; JITFALSE if it already exists
 */
JITBOOLEAN IRMETHOD_addInstructionControlDependence (ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst);

/**
 * \ingroup IRMETHOD_Program
 * @brief Return the list of IL binaries.
 *
 * Return the list of IL binaries currently loaded.
 *
 * The returned pointer points to the actual list maintained by ILDJIT.
 *
 * Hence, the caller cannot free it and every changes to it will be seen by both the caller and the callee.
 *
 * @return List of (t_binary_information *)
 */
XanList * IRPROGRAM_fetchILBinaries (void);

/**
 * \ingroup IRMETHOD_Program
 * @brief Check if a method belongs to a library
 *
 * Returns JITTRUE if the \c method belongs to an external library rather than the input program.
 *
 * Return JITFALSE otherwise.
 *
 * If ILDJIT cannot demonstrate that @c method belongs to an external library, then it returns JITFALSE.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRPROGRAM_doesMethodBelongToALibrary (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Program
 * @return List of (ir_method_t *)
 */
XanList * IRPROGRAM_getReachableMethodsNeededByProgram (void);

/**
 * \ingroup IRMETHOD_Program
 * @return List of (ir_method_t *)
 */
XanList * IRPROGRAM_getMethodsThatInitializeGlobalMemories (void);

/**
 * \ingroup IRMETHOD_Program
 * @brief Check if an instruction of a given type can be executed
 *
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRPROGRAM_isInstructionTypeReachable (ir_method_t *startMethod, JITUINT16 instructionType, XanList *escapedMethods, XanList *reachableMethods);

/**
 * \ingroup IRMETHOD_Program
 * @brief Return a set of methods callable from a given instruction.
 *
 * If the list pointed by <code> escapedMethods </code> is NULL, then this function will compute it internally.
 *
 * If the list pointed by <code> reachableMethodsFromEntryPoint </code> is NULL, then this function will compute it internally.
 *
 * @param inst Instruction to consider.
 * @param escapedMethods List of (ir_method_t *).
 * @param reachableMethodsFromEntryPoint List of (ir_method_t *).
 * @return List of IRMETHODID
 */
XanList * IRPROGRAM_getCallableMethods (ir_instruction_t *inst, XanList *escapedMethods, XanList *reachableMethodsFromEntryPoint);

/**
 * \ingroup IRMETHOD_Program
 * @brief Returns the methods reachable from a given one
 *
 * Returns the methods reachable from a given one.
 *
 * @param startMethod Method to consider as starting point
 * @return List of (ir_method_t *)
 */
XanList * IRPROGRAM_getReachableMethods (ir_method_t *startMethod);

/**
 * \ingroup IRMETHOD_Program
 * @brief Returns the list of escaped methods
 *
 * All methods that their addresses are taken directly, they are returned.
 *
 * @return List of IRMETHODID
 */
XanList * IRPROGRAM_getEscapedMethods (void);

/**
 * \ingroup IRMETHOD_Program
 * @brief Check if a method belongs to the input program.
 *
 * Returns JITTRUE if the \c method belongs to the input program.
 *
 * Return JITFALSE otherwise.
 *
 * If ILDJIT cannot demonstrate that @c method belongs to the input program, then it returns JITFALSE.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRPROGRAM_doesMethodBelongToProgram (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Program
 * @brief Check if a method belongs to the BCL library described in the standard ECMA 335
 *
 * Returns JITTRUE if the \c method belongs to the Base Class Library specified by the standard ECMA-335; JITFALSE otherwise.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRPROGRAM_doesMethodBelongToTheBaseClassLibrary (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Try to lock a IR method
 *
 * Returns 0 if the \c method is locked. 1 otherwise.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRMETHOD_tryLock (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Lock a IR method
 *
 * Lock the IR method given as input.
 *
 * @param method IR method to consider
 */
void IRMETHOD_lock (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Unlock a IR method
 *
 * Unlock the IR method given as input.
 *
 * @param method IR method to consider
 */
void IRMETHOD_unlock (ir_method_t *method);

/**
 * \ingroup IRMETHOD_DataTypeSize
 * @brief Get the number of bytes used for the type given as input
 *
 * Returns the number of bytes used to store instances of the type specified by the parameter \c item.
 *
 * In the case that item->type is equal to IRTYPE, the function returns the number of bytes needed to store a constant, or a variable, of type item->value.
 *
 * For example, if item->type is equal to IRTYPE and item->value is equal to IRINT32, then this function will return 4.
 *
 * @param item IR type
 * @return Number of bytes used for \c item
 */
JITINT32 IRDATA_getSizeOfType (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeSize
 * @brief Get the number of bytes used for the type given as input
 *
 * Returns the number of bytes used to store instances of the type specified by the parameter \c irType .
 *
 * This function does not handle \c IRVALUETYPE .
 *
 * @param irType IR type to consider
 * @return Number of bytes used for \c irType
 */
JITINT32 IRDATA_getSizeOfIRType (JITUINT16 irType);

/**
 * \ingroup IRMETHOD_DataTypeSize
 * @brief Get the number of bytes used for an object
 *
 * Returns the number of bytes used to allocate an allocated object specified by the parameter <code> item </code>.
 *
 * @param item IR type
 * @return Number of bytes used for object <code> item </code>
 */
JITINT32 IRDATA_getSizeOfObject (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeGlobal
 * @brief Check if a IR variable or constant is a global variable
 *
 * Check if a IR variable or a IR constant is a global variable.
 *
 * Following we get the effective address of the global variable:
 * @code
 * ir_item_t *par1;
 *
 * // Fetch a parameter
 * par1 = IRMETHOD_getInstructionParameter1(inst);
 *
 * // Check whether the parameter contains the address of a global variable
 * if (IRDATA_isAGlobalVariable(par1)){
 *	void 		*addr;
 *
 *	// Fetch the address
 *	addr	=  IRDATA_getGlobalVariableEffectiveAddress(par1);
 * }
 * @endcode
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isAGlobalVariable (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeGlobal
 * @brief Fetch the list of all globals.
 *
 * This function returns all globals registered at the time the call is made.
 *
 * The list must be freed by the caller by invoking \ref xanList_destroyList .
 *
 * Each element of the list is a pointer to \ref ir_global_t . These pointers actually point to the data structures that describe globals and that they will be used to generate machine code.
 *
 * @return List of (ir_global_t *)
 */
XanList * IRDATA_getGlobals (void);

/**
 * \ingroup IRMETHOD_DataTypeGlobal
 * @brief Get the address of a global variable
 *
 * Return the memory address where the global variable referred by @c item is stored.
 *
 * @param item IR variable or constant to check
 * @return Address of the global variable @c item
 */
void * IRDATA_getGlobalVariableEffectiveAddress (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeMethod
 * @brief Check if a IR variable or constant is a function pointer
 *
 * Check if a IR variable or a IR constant includes an address that is the entry point of a method.
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isAFunctionPointer (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeMethod
 * @brief Check if a IR variable or constant is an identificator of a method
 *
 * Check if a IR variable or a IR constant includes the identificator of a method.
 *
 * Notice that the identificator of a method and its entry point (the one checked by @c IRDATA_isAFunctionPointer) are different.
 *
 * In order to get the IR method from the method ID, you can:
 * @code
 * void my_do_job (ir_method_t *method){
 * ir_item_t *irItem;
 *
 * // Fetch an IR item
 * irItem = ...
 *
 * // Check whether an IR item has a method ID stored in it
 * if (IRDATA_isAMethodID(irItem)){
 *	IR_ITEM_VALUE	methodID;
 *
 *	// Fetch the method ID
 *	methodID	= IRDATA_getMethodIDFromItem(irItem);
 *
 *	// Fetch the IR method from the method ID
 *	irMethod	= IRMETHOD_getIRMethodFromMethodID(method, methodID);
 * }
 * }
 * @endcode
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isAMethodID (ir_item_t *item);

/**
 * \ingroup IRMETHOD_Method
 * @brief Get the method ID
 *
 * Return the method ID stored in the IR item @c item .
 *
 * This function returns 0 if @c item does not store any method .
 *
 * @param item IR variable or constant to check
 * @return Method ID
 */
IR_ITEM_VALUE IRDATA_getMethodIDFromItem (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeVariable
 * @brief Check if an IR item is a variable
 *
 * Check if an IR item is a variable.
 *
 * @param item IR item to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isAVariable (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeVariable
 * @brief Get the identifier of a variable
 *
 * Return the identifier of the variable @c item .
 *
 * @param item IR item to consider
 * @return ID of variable @c item
 */
IR_ITEM_VALUE IRDATA_getVariableID (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeSymbol
 * @brief Check if an IR item is a symbol
 *
 * Check if an IR item is a symbol.
 *
 * @param item IR item to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isASymbol (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeConstant
 * @brief Check if an IR item is a constant
 *
 * Check if an IR item is a constant.
 *
 * @param item IR item to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isAConstant (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeConstant
 * @brief Get the integer value of the constant
 *
 * Return the integer value of the constant stored in @c item .
 *
 * @param item IR item to consider
 * @return Integer constant
 */
IR_ITEM_VALUE IRDATA_getIntegerValueOfConstant (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeConstant
 * @brief Get the floating point value of the constant
 *
 * Return the floating point value of the constant stored in @c item .
 *
 * @param item IR item to consider
 * @return Floating point constant
 */
IR_ITEM_FVALUE IRDATA_getFloatValueOfConstant (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a type is platform dependent
 *
 * Check if the size of variables of type <code> type </code> depends on the underlying platform.
 *
 * @param type IR type to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAPlatformDependentType (JITUINT32 type);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a IR variable or constant is a float type
 *
 * Check if a IR variable or a IR constant if it stores a floating point value.
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasAFloatType (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a type is an float type
 *
 * Check if a type is an float type (either signed or unsigned)
 *
 * @param type IR type to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAFloatType (JITUINT32 type);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a IR variable or constant has an integer type
 *
 * Check if a IR variable or a IR constant if it stores a integer value.
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasAnIntType (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a type is an integer type
 *
 * Check if a type is an integer type (either signed or unsigned)
 *
 * @param type IR type to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAnIntType (JITUINT32 type);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a IR variable or constant has a data value type (a.k.a. data structure)
 *
 * Check if a IR variable or a IR constant if it stores an instance of a data value (a.k.a. data structure).
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasADataValueType (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a type is a data value type (a.k.a. data structure).
 *
 * Check if a type is a data value type (a.k.a. data structure).
 *
 * @param type IR type to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isADataValueType (JITUINT32 type);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a IR variable or constant has a pointer type
 *
 * Check if a IR variable or a IR constant if it stores a pointer value.
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasAPointerType (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a type is an pointer type
 *
 * Check if a type is an pointer type (either signed or unsigned)
 *
 * @param type IR type to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAPointerType (JITUINT32 type);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a IR variable or constant is a signed type
 *
 * Determine whether a parameter's type is signed or unsigned.
 *
 * Makes no assumptions about what parameters are given.
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasASignedType (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Check if a type is a signed type
 *
 * Determine whether a type is signed or unsigned.
 *
 * @param type IR type to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isASignedType (JITUINT32 type);

/**
 * \ingroup IRMETHOD_DataTypeMethod
 * @brief Check if a IR variable or constant is an entry point of a method
 *
 * Determine whether a parameter's type is a method entry point or not
 *
 * @param item IR variable or constant to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isAMethodEntryPoint (ir_item_t *item);

/**
 * \ingroup IRMETHOD_Program
 * @brief Get the entry point of the bytecode given as input
 *
 * Returns the first method of the CIL application that ILDJIT will execute.
 * Several languages (e.g. Java) call this method \c Main
 *
 * @return IR method executed as first
 */
ir_method_t * IRPROGRAM_getEntryPointMethod (void);

/**
 * \ingroup IRMETHOD_Program
 * @brief Get the set of methods with a given name
 *
 * Returns the set of methods that has the same name of @c methodName .
 *
 * The name of methods used to compare against @c methodName is the one returned by
 * @code
 * IRMETHOD_getMethodName
 * @endcode
 *
 * The caller is in charge to destroy the list returned.
 *
 * For example:
 * @code
 * XanList *l;
 *
 * // Fetch the methods with name "main"
 * l = IRPROGRAM_getMethodsWithName("main");
 *
 * // Use the methods
 * item = xanList_first(l);
 * while (item != NULL){
 *	ir_method_t	*m;
 *
 *	// Fetch a method
 *	m	= item->data;
 *	assert(strcmp(IRMETHOD_getMethodName(m), "main") == 0);
 *
 *	// Use the method
 *	...
 *
 *	// Fetch the next element from the list
 * 	item	= item->next;
 * }
 *
 * // Free the memory used by the list
 * xanList_destroyList(l);
 * @endcode
 *
 * @return List of (ir_method_t *)
 */
XanList * IRPROGRAM_getMethodsWithName (JITINT8 *methodName);

/**
 * \ingroup IRMETHOD_Program
 * @brief Get the method with a given signature in string
 *
 * Returns the method that has the signature in string equals to the one given as input.
 *
 * If this method exists, it is unique.
 * If this methods does not exist, this function returns NULL.
 *
 * The string compared against @c signatureInString is the one returned by
 * @code
 * IRMETHOD_getSignatureInString
 * @endcode
 *
 * @param signatureInString String representation of the signature of the returned method.
 * @return Method with the signature specified
 */
ir_method_t * IRPROGRAM_getMethodWithSignatureInString (JITINT8 *signatureInString);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the instruction belongs to a method
 *
 * Returns JITTRUE if the instruction \c inst belongs to the method \c method; JITFALSE otherwise.
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doesInstructionBelongToMethod (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if instructions belong to a method
 *
 * Returns JITTRUE if instructions \c insts belongs to the method \c method; JITFALSE otherwise.
 *
 * @param method IR method to consider
 * @param insts IR instructions to consider. List of (ir_instruction_t *)
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doInstructionsBelongToMethod (ir_method_t *method, XanList *insts);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction is a call to another method
 *
 * Return JITTRUE if the instruction \c inst is a call instruction, which means that the program counter changes not in the normal way after its execution
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isACallInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction may perform a memory store instruction
 *
 * Return JITTRUE if the instruction <code> inst </code> may perform a store to the memory.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_canInstructionPerformAMemoryStore (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction may perform a memory load instruction
 *
 * Return JITTRUE if the instruction <code> inst </code> may perform a load from the memory.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_canInstructionPerformAMemoryLoad (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Linkage
 * @brief Check if a method can be called outside the IR code
 *
 * Check if a method can be called outside the IR code.
 *
 * IR methods can be called from normal C code in two ways (assuming the IR method takes an integer as parameter):
 * @code
 * void *fp;
 * int ret;
 * int inputValue;
 * inputValue = 5;
 *
 * fp = IRMETHOD_getFunctionPointer(method);
 *
 * ret = (*fp)(inputValue);
 * @endcode
 * or
 * @code
 * int ret;
 * int input;
 * void *args[1];
 * input = 5;
 * args[0] = &input;
 *
 * IRMETHOD_executeMethod(method, args, &ret);
 * @endcode
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isCallableExternally (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Linkage
 * @brief Set or unset a method to be called externally
 *
 * Set or unset a method to be called externally
 *
 * @param method IR method to consider
 * @param callableExternally Is @c method callable externally?
 */
void IRMETHOD_setMethodAsCallableExternally (ir_method_t *method, JITBOOLEAN callableExternally);

/**
 * \ingroup IRMETHOD_Method
 * @brief Get a IR method stored to a given memory address
 *
 * Return the IR Method that has \c entryPointAddress as memory address of the first assembly instruction.
 *
 * @param entryPointAddress Memory address where the IR method is stored
 * @return IR method that is stored starting from the address \c entryPointAddress
 */
ir_method_t * IRMETHOD_getIRMethodFromEntryPoint (void *entryPointAddress);

/**
 * \ingroup IRMETHOD_MethodBody
 * @brief Check if a method can have a IR body
 *
 * Check if the method <code> method </code> can have an IR body.
 *
 * For example, methods that cannot be make concrete cannot have IR bodies.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_canHaveIRBody (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method
 * @brief Check if a method is used to initialize memory
 *
 * @brief Check if the method @c method is used to initialize memory.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doesMethodInitializeGlobalMemory (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionCalledMethod
 * @brief Get a native method called by an instruction
 *
 * Return the C function (native method) called by the instruction <code> callInstruction </code>.
 *
 * In case the instruction does not call any native method, it returns NULL.
 *
 * @param callInstruction IR instruction to consider
 * @return Pointer to the C function called by <code> callInstruction </code>
 */
void * IRMETHOD_getCalledNativeMethod (ir_instruction_t *callInstruction);

/**
 * \ingroup IRMETHOD_InstructionCalledMethod
 * @brief Get the name of the method called by an instruction
 *
 * Return the name (C string) of the method called by the instruction <code> callInstruction </code>.
 *
 * The returned pointer cannot be freed by the caller.
 *
 * In case the instruction does not call any method, it returns NULL.
 *
 * For example:
 * @code
 * printf("Called method = %s", IRMETHOD_getNameOfCalledMethod(callInstruction));
 * @endcode
 *
 * @param method Method that includes @c callInstruction
 * @param callInstruction IR instruction to consider
 * @return Pointer to the C string.
 */
JITINT8 * IRMETHOD_getNameOfCalledMethod (ir_method_t *method, ir_instruction_t *callInstruction);

/**
 * \ingroup IRMETHOD_InstructionCalledMethod
 * @brief Get a IR method called by an instruction
 *
 * Return the IR Method that is called by the instruction \c callInstruction.
 *
 * @param method IR method that includes the call instruction \c callInstruction
 * @param callInstruction IR instruction to consider
 * @return IR method that is called by \c callInstruction
 */
ir_method_t * IRMETHOD_getCalledMethod (ir_method_t *method, ir_instruction_t *callInstruction);

/**
 * \ingroup IRMETHOD_Method
 * @brief Get a IR method from an ID
 *
 * This function returns the IR method with the ID specified as input.
 * This ID is usually stored within parameters of IR instructions like IRCALL, IRVCALL etc...
 *
 * @param method Method that the caller is considering (not necessary the method with \c irMethodID as ID but the method had as its input)
 * @param irMethodID Identificator of the IR method returned
 * @return IR method that has irMethodID has its ID
 */
ir_method_t * IRMETHOD_getIRMethodFromMethodID (ir_method_t *method, IR_ITEM_VALUE irMethodID);

/**
 * \ingroup IRMETHOD_Program
 * @brief Return the list containing the available implementation of virtual method
 *
 * Return the list containing the IR Method of the available implementation of virtual method.
 * The ID is usually store within parameters of IR instruction IRVCALL.
 *
 * The result is a list of IRMETHODID, which means you need to call the function \ref IRMETHOD_getIRMethodFromMethodID in order to have the IR method from the elements of the list.
 *
 * @param irMethodID Method to consider
 * @return List of IRMETHODID
 */
XanList * IRPROGRAM_getImplementationsOfMethod (IR_ITEM_VALUE irMethodID);

/**
 * \ingroup IRMETHOD_Program
 * @brief Return the list of methods with the signature given as input
 *
 * Return the list of methods with the signature given as input.
 *
 * @param signature Signature to consider
 * @return List of IRMETHODID
 */
XanList * IRPROGRAM_getCompatibleMethods (ir_signature_t *signature);

/**
 * \ingroup IRMETHOD_InstructionCalledMethod
 * @brief Return a list of callable methods by the input instruction
 *
 * Return the list containing the methods callable by the instruction \c inst given as input.
 *
 * The result is a list of IRMETHODID, which means you need to call the function \ref IRMETHOD_getIRMethodFromMethodID in order to have the IR method from the elements of the list.
 *
 * The caller needs to free the memory of the list (by calling the function \ref xanList_destroyList of the returned list) when it is not useful anymore.
 *
 * @param inst IR instruction that can call every method within the list returned
 * @return List of IRMETHODID
 */
XanList * IRMETHOD_getCallableMethods (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCalledMethod
 * @brief Return a list of callable IR methods by the input instruction
 *
 * Return the list containing the IR methods callable by the instruction \c inst given as input.
 *
 * The result is a list of (ir_method_t *).
 *
 * @param inst IR instruction that can call every method within the list returned
 * @return List of (ir_method_t *)
 */
XanList * IRMETHOD_getCallableIRMethods (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_CodeGeneration
 * @brief Translate a method to the IR language
 *
 * This function ensures that the method \c method given as input is available in IR language after its call.
 * If the method is already available in IR, no action is performed.
 * Otherwise, the method is translated from the source language to the IR one.
 *
 * As the function \ref IRMETHOD_createMethodTrampoline, this one can be called after the IR signature is provided.
 *
 * @param method Method to consider
 */
void IRMETHOD_translateMethodToIR (ir_method_t *method);

/**
 * \ingroup IRMETHOD_CodeGeneration
 * @brief Translate a method to the machine code
 *
 * This function translates the IR method \c method to the machine code of the underlying platform.
 * If the method was already compiled to machine code, by calling this function, you are retranslating the IR method to the machine code.
 *
 * Notice that the <code> method </code> is ready to be executed when this function returns.
 *
 * @param method IR method to translate
 */
void IRMETHOD_translateMethodToMachineCode (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method
 * @brief Get the method ID of a IR method
 *
 * Return the ID of the IR method \c method given as input.
 * The return value could be use everywhere a IRMETHODID is requested (e.g. within IRCALL instructions)
 *
 * @param method IR method to consider
 * @return IRMETHODID of the IR method given as input
 */
IR_ITEM_VALUE IRMETHOD_getMethodID (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Add the method to the list of profiled ones
 *
 * Add method <code> method </code> to the list of methods profiled.
 *
 * This information can be used by profile-based code translations or optimizations like the partial AOT.
 *
 * @param method IR method to add
 */
void IRMETHOD_tagMethodAsProfiled (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Program
 * @brief Get the list of all IR methods
 *
 * Return the list of methods that have been compiled already by ILDJIT.
 *
 * Every element of the list is of type (ir_method_t *).
 *
 * The caller is in charge to free the memory of the list as the following code shows:
 * @code
 * XanList *methods;
 *
 * // Fetch all methods
 * methods = IRPROGRAM_getIRMethods();
 *
 * // Use all methods
 * ...
 *
 * // Free the memory
 * xanList_destroyList(methods);
 *
 * @endcode
 *
 * @return List of (ir_method_t *)
 */
XanList * IRPROGRAM_getIRMethods (void);

/**
 * \ingroup IRMETHOD_Method
 * @brief Return the descriptor of the method.
 *
 * Return the high level descriptor of the method.
 * This descriptor includes information that came from the source bytecode language (e.g. CIL)
 *
 * @param method IR method to consider
 * @return Descriptor of the IR method \c method
 */
MethodDescriptor * IRMETHOD_getMethodDescription (ir_method_t * method);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction may access to the memory of the program
 *
 * Return JITTRUE if the instruction \c inst may access to memory (heap or stack); return JITFALSE otherwise.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_mayAccessMemory (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the address of the memory location accessed
 *
 * Store into @c addr the address of the memory location accessed by the instruction @c inst .
 *
 * If the address is unknown (e.g. it is computed at runtime), then @c addr is initialized to NOPARAM (i.e., addr->type == NOPARAM).
 *
 * This is an example of use:
 * @code
 * ir_item_t mAddr;
 *
 * IRMETHOD_addressOfAccessedMemoryLocation(inst, &mAddr);
 * if (mAddr.type != NOPARAM){
 *   printf("The address is %p\n", (void *)(JITNUINT)mAddr.value.v);
 * }
 * @endcode
 *
 * @param inst Instruction to consider
 * @param addr Item where to store the address
 */
void IRMETHOD_addressOfAccessedMemoryLocation (ir_instruction_t *inst, ir_item_t *addr);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the address of the memory location accessed
 *
 * Store into @c ba and @c offset the base address and the offset from it of the memory location accessed by the instruction @c inst .
 *
 * If the base address is unknown (e.g. it is computed at runtime), then @c ba is initialized to NOPARAM (i.e., ba->type == NOPARAM).
 *
 * This is an example of use:
 * @code
 * ir_item_t ba;
 * ir_item_t o;
 *
 * IRMETHOD_baseAddressAndOffsetOfAccessedMemoryLocation(inst, &ba, &o);
 * if (ba.type != NOPARAM){
 *   printf("The base address is known at compile time\n");
 * }
 * @endcode
 *
 * @param inst Instruction to consider
 * @param ba Item where to store the base address
 * @param offset Item where to store the offset
 */
void IRMETHOD_baseAddressAndOffsetOfAccessedMemoryLocation (ir_instruction_t *inst, ir_item_t *ba, ir_item_t *offset);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the result of an instruction
 *
 * Return the result of the instruction.
 *
 * @param inst Instruction to consider
 * @return Result of <code> inst </code>
 */
ir_item_t * IRMETHOD_getInstructionResult (ir_instruction_t * inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the type of the result of an instruction
 *
 * Return the type of the result of the IR instruction given as input.
 *
 * @param inst IR instruction to consider
 * @return IR type of the result of the instruction (e.g. IRSTRING, IROFFSET, etc...)
 */
JITUINT32 IRMETHOD_getInstructionResultType (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the value of the result of an instruction
 *
 * Return the value of the result of the IR instruction given as input.
 * This is the identifier of the variable where the result will be eventually stored.
 *
 * @param inst IR instruction to consider
 * @return IR value of the result of the instruction (e.g. 5)
 */
IR_ITEM_VALUE IRMETHOD_getInstructionResultValue (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the internal type of the result of an instruction
 *
 * Return the internal type of the result of the IR instruction given as input.
 *
 * @param inst IR instruction to consider
 * @return IR internal type of the result of the instruction (e.g. IRSTRING)
 */
JITUINT32 IRMETHOD_getInstructionResultInternalType (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the IR variable used within a method
 *
 * Return the ir_item_t used within an instruction of the method given as input that represent the IR variable \c varID.
 *
 * @param method Method that includes the variable \c varID.
 * @param varID Variable ID to search.
 * @return The IR variable information.
 */
ir_item_t * IRMETHOD_getIRVariable (ir_method_t *method, JITUINT32 varID);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get IR variables used within a method
 *
 * Return the set of variables used by the method @c method.
 *
 * The caller is in charge to free the memory returned as following shown:
 * @code
 * ir_item_t **vars;
 * vars = IRMETHOD_getIRVariables(method);
 *
 * // Use variable 5
 * printf("Variable %u\n", (vars[5]->value).v);
 *
 * freeFunction(vars);
 * @endcode
 *
 * @param method Method to consider.
 * @return IR variables used by @c method.
 */
ir_item_t ** IRMETHOD_getIRVariables (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Check if a parameter of an instruction is a variable
 *
 * Return JITTRUE if the parameter <code> parameterNumber </code> of the instruction <code> inst </code> is a variable; return JITFALSE otherwise.
 *
 * @param inst IR instruction to consider
 * @param parameterNumber Number of the parameter to consider (it can be 3, 2 or 1)
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isInstructionParameterAVariable (ir_instruction_t *inst, JITUINT8 parameterNumber);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Check if a parameter of an instruction is a constant
 *
 * Return JITTRUE if the parameter <code> parameterNumber </code> of the instruction <code> inst </code> is a constant; return JITFALSE otherwise.
 *
 * The parameter 0 is the return value of the instruction.
 *
 * @param inst IR instruction to consider
 * @param parameterNumber Number of the parameter to consider (it can be 3, 2, 1 or 0)
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isInstructionParameterAConstant (ir_instruction_t *inst, JITUINT8 parameterNumber);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Add a new call parameter to an instruction
 *
 * Add a new call parameter cloning <code> newCallParameter </code>.
 *
 * The new call parameter is added to the instruction <code> inst </code>.
 *
 * For example, the following code adds a new call parameter to an existing call instruction:
 * @code
 * ir_item_t newParam;
 * memset(&newParam, 0, sizeof(ir_item_t));
 * newParam.value.v = 5;
 * newParam.type = IRINT32;
 * newParam.internal_type = newParam.type;
 * IRMETHOD_addInstructionCallParameter(method, callInstruction, &newParam);
 * @endcode
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction that includes the nth call paramter
 * @param newCallParameter The new call parameter to clone
 */
void IRMETHOD_addInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, ir_item_t *newCallParameter);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Copy an IR item to a call parameter of an instruction
 *
 * Copy an already existing parameter \c from into the \c nth call parameter of the IR instruction \c inst.
 *
 * Call parameters start from \c 0 .
 *
 * The \c nth call parameter must exist before this function is invoked.
 *
 * There is no constrain about the IR item \c from : it could be part of any IR instruction.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction that includes the nth call paramter
 * @param nth Number of the call parameter to consider
 * @param from IR item to copy
 */
void IRMETHOD_cpInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth, ir_item_t *from);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Set a parameter or result of an instruction
 *
 * Set the call parameter <code> parameterNumber </code> of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * The first call parameter is <code> 0 </code>.
 *
 * The <code> typeInfos </code> can be <code> NULL </code> if no information is known.
 *
 * The call parameter must already exist.
 *
 * @param method IR Method that contain the instruction <code> fromInst </code>
 * @param inst IR instruction to consider
 * @param value Integer constant or variable ID to use to set the call parameter <code> parameterNumber </code>
 * @param fvalue Floating point constant to use to set the call parameter <code> parameterNumber </code>
 * @param type Type of the constant or IROFFSET if it is a variable to use to set the call parameter <code> parameterNumber </code>
 * @param internal_type Type of the constant or variable to use to set the call parameter <code> parameterNumber </code>
 * @param typeInfos Information about the type of the call parameter to set
 * @param parameterNumber Number of the call parameter to set
 */
void IRMETHOD_setInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos, JITUINT32 parameterNumber);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Create a new call parameter of an instruction
 *
 * Create a new call parameter of the instruction <code> inst </code>, which belongs to the method <code> method </code>.
 *
 * This function returns the position of the new call parameter inserted; positions start from 0.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction that includes the nth call paramter
 * @return The position of the new call parameter inserted
 */
JITUINT32 IRMETHOD_newInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Change the position of an already existing instruction parameter
 *
 * This function moves the already existing call parameter of the instruction \c inst from the position \c oldPos to the position \c newPos.
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @param oldPos Position of \c inst before calling this function
 * @param oldPos Position of \c inst after calling this function
 */
void IRMETHOD_moveInstructionCallParameter (ir_method_t * method, ir_instruction_t *inst, JITUINT32 oldPos, JITUINT32 newPos);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Check if the method contains circuits
 *
 * Check if the method <code> method </code> contains at least one circuit.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasMethodCircuits (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get the set of circuits of a method
 *
 * Return a new allocated list containing the set of circuits that belong to the method given as input (i.e., \c method ).
 *
 * The returned list must be destroyed by the caller by calling \ref xanList_destroyList .
 *
 * If the method contains no circuit, the returned list is empty and it still requires to be destroyed by the caller.
 *
 * @param method Method to consider
 * @return List of (circuit_t *)
 */
XanList * IRMETHOD_getMethodCircuits (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get the more nested circuit of an instruction
 *
 * Return the more nested circuit where the instruction \c inst belongs to.
 * Return NULL if \c inst does not belong to any circuit.
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @return The more nested circuit of \c inst
 */
circuit_t * IRMETHOD_getTheMoreNestedCircuitOfInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get the immediate nested circuits of the one given as input
 *
 * Get the set of circuits that has the one given as input as its immediate parent within the circuit nesting tree of the method given as input.
 * The caller needs to free the list (by calling the function \ref xanList_destroyList of the XanList structure) after he does not need it anymore.
 *
 * @param method IR method that containts the input circuit
 * @param circuit Circuit to consider
 * @return List of (circuit_t *)
 */
XanList * IRMETHOD_getImmediateNestedCircuits (ir_method_t *method, circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get the nested circuits of the one given as input
 *
 * Get every nested circuit of the one given as input.
 *
 * The caller needs to free the list (by calling the function \ref xanList_destroyList of the XanList structure) after he does not need it anymore.
 *
 * @param method IR method that containts the input circuit
 * @param circuit Circuit to consider
 * @return List of (circuit_t *)
 */
XanList * IRMETHOD_getNestedCircuits (ir_method_t *method, circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get outermost circuits
 *
 * Return the list of outermost circuits of the method \c method.
 * The caller of this function has to free the list when he does not need it anymore.
 * Before calling the method, be sure that the information LOOP_IDENTIFICATION is already computed before.
 *
 * @param method IR method to consider
 * @return List of (circuit_t *)
 */
XanList * IRMETHOD_getOutermostCircuits (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get the parent circuit
 *
 * Return the parent circuit of the one (i.e., \c circuit) given as input.
 * Return NULL if the circuit given as input is the outest one.
 *
 * @param method IR method where \c circuit belongs to
 * @param circuit Circuit to consider
 * @return Parent circuit of \c circuit or NULL
 */
circuit_t * IRMETHOD_getParentCircuit (ir_method_t *method, circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get instructions belong to a circuit
 *
 * Return a list of instructions belong to the circuit given in input.
 * The list returned has to be destroied after its usage by the caller (using the function \ref xanList_destroyList of the XanList structure).
 *
 * @param method IR method where the circuit belongs to
 * @param circuit Circuit to consider
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getCircuitInstructions (ir_method_t* method, circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit_Backedge
 * @brief Get the source of the backedge of a circuit
 *
 * Return the source of the backedge of the circuit given as input (i.e., \c circuit ).
 *
 * @param circuit Circuit to consider
 * @return The source of the backedge of the circuit
 */
ir_instruction_t * IRMETHOD_getTheSourceOfTheCircuitBackedge (circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit_Backedge
 * @brief Get the destination of the backedge of a circuit
 *
 * Return the destination of the backedge of the circuit given as input (i.e., \c circuit ).
 *
 * The destination of the backedge is the first instruction executed within the circuit.
 *
 * @param circuit Circuit to consider
 * @return The destination of the backedge of the circuit
 */
ir_instruction_t * IRMETHOD_getTheDestinationOfTheCircuitBackedge (circuit_t *circuit);

/**
 * \ingroup IRMETHOD_InstructionDominance
 * @brief Get the set of postdominators of an instruction
 *
 * Return the list of post dominators of the instruction \c inst.
 *
 * Before calling the method, be sure that the information POST_DOMINATOR_COMPUTER is already computed before.
 *
 * @param method IR Method where the instruction \c inst belongs to
 * @param inst IR instruction that belongs to \c method
 * @return List of postdominators of \c inst
 */
XanList * IRMETHOD_getPostDominators (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionDominance
 * @brief Get the set of predominators of an instruction
 *
 * Return the list of pre dominators of the instruction \c inst.
 *
 * Before calling the method, be sure that the information PRE_DOMINATOR_COMPUTER is already computed before.
 *
 * @param method IR Method where the instruction \c inst belongs to
 * @param inst IR instruction that belongs to \c method
 * @return List of predominators of \c inst
 */
XanList * IRMETHOD_getPreDominators (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the catcher instruction
 *
 * Return the catcher IR instruction of the method, if it is present, NULL otherwise.
 *
 * @param method IR method to consider
 * @return The catcher instruction of the method
 */
ir_instruction_t * IRMETHOD_getCatcherInstruction (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method
 * @brief Check if a method has blocks to catch exceptions.
 *
 * Check if a method has blocks to catch exceptions that could be thrown by the method or any called ones.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasCatchBlocks (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the successor of an instruction
 *
 * If the prevSuccessor paramater is equal to NULL, then this method returns the IR instruction which is the first successor in the Control Flow Graph of the instruction described by the inst parameter, otherwise it returns the next successor of \c inst.
 *
 * @param method Method that includes @c inst.
 * @param inst IR instruction to consider
 * @param prevSuccessor Previous successor returned by this method.
 * @return Successor of @c inst.
 */
ir_instruction_t * IRMETHOD_getSuccessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevSuccessor);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the predecessor of an instruction
 *
 * If @c prevPredecessor is equal to NULL, then this method returns the IR instruction which is the first predecessor in the Control Flow Graph of the instruction described by the parameter @c inst.
 * Otherwise this method returns the next predecessor of @c inst.
 *
 * @param method Method that includes @c inst.
 * @param inst IR instruction to consider
 * @param prevPredecessor Previous predecessor returned by this method.
 * @return Predecessor of @c inst.
 */
ir_instruction_t * IRMETHOD_getPredecessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevPredecessor);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the set of predecessors of the instruction.
 *
 * Get the set of predecessors of the instruction.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst Instruction to consider
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getInstructionPredecessors (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Check whether an instruction can be the last one executed in the method.
 *
 * Check whether the instruction @c inst be the last one executed in a given invocation of a method.
 *
 * A simple example of an instruction with this property is a \ref IRRET instruction.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst Instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_canInstructionBeTheLastOneExecutedInTheMethod (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the set of predecessors of every instruction.
 *
 * Get the set of predecessors of every instruction of the method @c method .
 *
 * The returned set of lists can be used as following:
 * @code
 * XanListItem *item;
 * XanList **p = IRMETHOD_getInstructionsPredecessors(method);
 * JITUINT32 n = IRMETHOD_getInstructionsNumber(method);
 *
 * // Iterate over predecessors
 * item = xanList_first(p[inst->ID]);
 * while (item != NULL){
 * 	ir_instruction_t *pred1 = item->data;
 *	item = item->next;
 * }
 *
 * // Free the memory
 * IRMETHOD_destroyInstructionsPredecessors(p, n);
 * @endcode
 *
 * @param method IR method that includes the instruction \c inst
 * @return List of (XanList *) where each element of each list is (ir_instruction_t *)
 */
XanList ** IRMETHOD_getInstructionsPredecessors (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the set of successors of every instruction.
 *
 * Get the set of successors of every instruction of the method @c method .
 *
 * The returned set of lists can be used as following:
 * @code
 * XanListItem *item;
 * XanList **s = IRMETHOD_getInstructionsSuccessors(method);
 * JITUINT32 n = IRMETHOD_getInstructionsNumber(method);
 *
 * // Fetch the first successor
 * item = xanList_first(s[inst->ID]);
 * ir_instruction_t *succ1 = (item != NULL) ? item->data : NULL;
 *
 * // Fetch the second successor
 * item = (item != NULL) ? item->next : NULL;
 * ir_instruction_t *succ2 = (item != NULL) ? item->data : NULL;
 *
 * // Free the memory
 * IRMETHOD_destroyInstructionsSuccessors(s, n);
 * @endcode
 *
 * @param method IR method that includes the instruction \c inst
 * @return List of (XanList *) where each element of each list is (ir_instruction_t *)
 */
XanList ** IRMETHOD_getInstructionsSuccessors (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Destroy cached predecessors of instructions.
 *
 * Destroy cached predecessors of instructions of a method.
 *
 * The parameter @c predecessors must be the output of a previous call to
 * @code
 * IRMETHOD_getInstructionsPredecessors
 * @endcode
 *
 * For example, the following code shows the use case related to it:
 * @code
 * XanList **p = IRMETHOD_getInstructionsPredecessors(method);
 * JITUINT32 n = IRMETHOD_getInstructionsNumber(method);
 *
 * // Uses of p
 *
 * IRMETHOD_destroyInstructionsPredecessors(p, n);
 * @endcode
 *
 * @param predecessors List of (XanList *) where each element of each list is (ir_instruction_t *)
 * @param instructionsNumber Number of instructions used to cache the predecessors
 */
void IRMETHOD_destroyInstructionsPredecessors (XanList **predecessors, JITUINT32 instructionsNumber);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Refresh cached predecessors of instructions.
 *
 * Refresh cached predecessors of instructions of a method.
 *
 * The parameter @c predecessors must be the output of a previous call to
 * @code
 * IRMETHOD_getInstructionsPredecessors
 * @endcode
 *
 * For example, the following code shows the use case related to it:
 * @code
 * XanList **p = IRMETHOD_getInstructionsPredecessors(method);
 * JITUINT32 n = IRMETHOD_getInstructionsNumber(method);
 *
 * // Uses of p
 * // Changes to the CFG
 *
 * IRMETHOD_refreshInstructionsPredecessors(method, &p, &n);
 *
 * // Uses of p
 *
 * IRMETHOD_destroyInstructionsPredecessors(p, n);
 * @endcode
 *
 * @param method Method that contains the instructions stored in @c predecessors
 * @param predecessors List of (XanList *) where each element of each list is (ir_instruction_t *)
 * @param instructionsNumber Pointer to the variable that contain the number of instructions used to cache the predecessors
 */
void IRMETHOD_refreshInstructionsPredecessors (ir_method_t *method, XanList ***predecessors, JITUINT32 *instructionsNumber);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Destroy cached successors of instructions.
 *
 * Destroy cached successors of instructions of a method.
 *
 * The parameter @c successors must be the output of a previous call to
 * @code
 * IRMETHOD_getInstructionsSuccessors
 * @endcode
 *
 * For example, the following code shows the use case related to it:
 * @code
 * XanList **s = IRMETHOD_getInstructionsSuccessors(method);
 * JITUINT32 n = IRMETHOD_getInstructionsNumber(method);
 *
 * // Uses of s
 *
 * IRMETHOD_destroyInstructionsSuccessors(s, n);
 * @endcode
 *
 * @param successors List of (XanList *) where each element of each list is (ir_instruction_t *)
 * @param instructionsNumber Number of instructions used to cache the successors
 */
void IRMETHOD_destroyInstructionsSuccessors (XanList **successors, JITUINT32 instructionsNumber);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Refresh cached successors of instructions.
 *
 * Refresh cached successors of instructions of a method.
 *
 * The parameter @c successors must be the output of a previous call to
 * @code
 * IRMETHOD_getInstructionsSuccessors
 * @endcode
 *
 * For example, the following code shows the use case related to it:
 * @code
 * XanList **p = IRMETHOD_getInstructionsSuccessors(method);
 * JITUINT32 n = IRMETHOD_getInstructionsNumber(method);
 *
 * // Uses of p
 * // Changes to the CFG
 *
 * IRMETHOD_refreshInstructionsSuccessors(method, &p, &n);
 *
 * // Uses of p
 *
 * IRMETHOD_destroyInstructionsSuccessors(p, n);
 * @endcode
 *
 * @param method Method that contains the instructions stored in @c successors
 * @param successors List of (XanList *) where each element of each list is (ir_instruction_t *)
 * @param instructionsNumber Pointer to the variable that contain the number of instructions used to cache the successors
 */
void IRMETHOD_refreshInstructionsSuccessors (ir_method_t *method, XanList ***successors, JITUINT32 *instructionsNumber);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the set of successors of the instruction.
 *
 * Get the set of successors of the instruction.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst Instruction to consider
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getInstructionSuccessors (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ModifyMethodsMovingInstructions
 * @brief Move an instruction just before another one
 *
 * Move the instruction \c inst just before the instruction \c prev
 *
 * @param method IR method that includes both instructions \c inst and \c prev
 * @param inst Instruction to move
 * @param prev Instruction that will be just after \c inst
 */
void IRMETHOD_moveInstructionBefore (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prev);

/**
 * \ingroup IRMETHOD_ModifyMethodsMovingInstructions
 * @brief Move an instruction after another one
 *
 * Move the instruction \c inst just after the instruction \c succ given as input.
 *
 * @param method IR method that includes both instructions \c inst and \c succ
 * @param inst Instruction to move
 * @param succ Instruction that will be just before \c inst
 */
void IRMETHOD_moveInstructionAfter (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *succ);

/**
 * \ingroup IRMETHOD_ModifyMethodsMovingInstructions
 * @brief Schedule an instruction just before another one
 *
 * Schedule the instruction <code> inst </code> just before the instruction <code> prev </code>
 */
void IRMETHOD_scheduleInstructionBefore (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prev);

/**
 * \ingroup IRMETHOD_ModifyLabels
 * @brief Get a new label ID
 *
 * Return a free label ID to use.
 *
 * Probably, after calling this function you will want to add a new IRLABEL instruction to fix the returned label ID.
 *
 * Use this ID within some IR instruction of the method before calling this function again.
 *
 * See also:
 * <ul>
 * <li> \ref IRMETHOD_newLabel
 * <li> \ref IRMETHOD_newLabelBefore
 * <li> \ref IRMETHOD_newLabelAfter
 * </ul>
 *
 * @param method IR method to consider
 * @return A new label ID not yet used by the IR method given as input
 */
IR_ITEM_VALUE IRMETHOD_newLabelID (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Add a new instruction
 *
 * Allocate a new IR instruction to \c method and move it at the end of that method.
 *
 * The new instruction is returned.
 *
 * @param method IR method where the new returned instruction is added
 * @return New instruction appended to <code> method </code>
 */
ir_instruction_t * IRMETHOD_newInstruction (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Add a new instruction before the one given as input
 *
 * Allocate a new IR instruction to the method and move it just before the instruction given as input
 *
 * @param method IR method that includes the instruction \c prev
 * @param prev IR instruction stored just after the one returned
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newInstructionBefore (ir_method_t *method, ir_instruction_t *prev);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Add a new instruction after the one given as input
 *
 * Allocate a new IR instruction to the method and move it just after the instruction given as input
 *
 * @param method IR method that includes the instruction \c prev
 * @param prev IR instruction stored just before the one returned
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newInstructionAfter (ir_method_t *method, ir_instruction_t *prev);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Add a new instruction
 *
 * Allocate a new IR instruction of type <code> instructionType </code> to the IR method <code> method </code> and move it at the end of that method.
 *
 * The new instruction is returned.
 *
 * @param method IR method where the new returned instruction is added
 * @param instructionType Type of the returned instruction
 * @return New instruction of type <code> instructionType </code> appended to <code> method </code>
 */
ir_instruction_t * IRMETHOD_newInstructionOfType (ir_method_t *method, JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Add a new instruction
 *
 * Allocate a new IR instruction of type <code> instructionType </code> to the IR method <code> method </code> and move it just before the instruction <code> prev </code> given as input.
 *
 * The new instruction is returned.
 *
 * If @c prev is NULL, then the behaviour is equivalent to \ref IRMETHOD_newInstructionOfType.
 *
 * @param method IR method where the new returned instruction is added
 * @param instructionType Type of the returned instruction
 * @param prev IR instruction stored just after the one returned
 * @return New instruction of type <code> instructionType </code> appended to <code> method </code>
 */
ir_instruction_t * IRMETHOD_newInstructionOfTypeBefore (ir_method_t *method, ir_instruction_t *prev, JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Add a new instruction
 *
 * Allocate a new IR instruction of type <code> instructionType </code> to the IR method <code> method </code> and move it just after the instruction <code> prev </code> given as input.
 *
 * The new instruction is returned.
 *
 * If @c prev is NULL, then the behaviour is equivalent to \ref IRMETHOD_newInstructionOfType.
 *
 * @param method IR method where the new returned instruction is added
 * @param instructionType Type of the returned instruction
 * @param prev IR instruction stored just before the one returned
 * @return New instruction of type <code> instructionType </code> appended to <code> method </code>
 */
ir_instruction_t * IRMETHOD_newInstructionOfTypeAfter (ir_method_t *method, ir_instruction_t *prev, JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingLabelInstructions
 * @brief Add a new label instruction
 *
 * Return a new label instruction (IRLABEL).
 * The returned instruction is placed at the bottom of the method.
 *
 * @param method IR method to consider
 * @return A new IRLABEL instruciton not yet used by the IR method given as input
 */
ir_instruction_t * IRMETHOD_newLabel (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingLabelInstructions
 * @brief Add a new label instruction
 *
 * Return a new label instruction (IRLABEL) inserted just before <code> beforeInstruction </code>.
 *
 * @param method IR method to consider
 * @param beforeInstruction The returned instruction is stored just before this one
 * @return A new IRLABEL instruciton not yet used by the IR method given as input
 */
ir_instruction_t * IRMETHOD_newLabelBefore (ir_method_t *method, ir_instruction_t *beforeInstruction);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingLabelInstructions
 * @brief Add a new label instruction
 *
 * Return a new label instruction (IRLABEL) inserted just after <code> afterInstruction </code>.
 *
 * @param method IR method to consider
 * @param afterInstruction The returned instruction is stored just after this one
 * @return A new IRLABEL instruciton not yet used by the IR method given as input
 */
ir_instruction_t * IRMETHOD_newLabelAfter (ir_method_t *method, ir_instruction_t *afterInstruction);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Set an instruction as to be a new label
 *
 * Set the instruction <code> inst </code> to be a new label.
 *
 * @param method IR method to consider
 * @param inst Instruction to transform to a new label
 */
void IRMETHOD_setInstructionToANewLabel (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_MethodLabels
 * @brief Get the maximum label ID
 *
 * Return the maximum label ID used within the IR method given as input.
 *
 * @param method IR method to consider
 */
IR_ITEM_VALUE IRMETHOD_getMaxLabelID (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingBranchInstructions
 * @brief Add a new branch to the label given as input
 *
 * Add a new branch instruction (i.e., IRBRANCH) to the label given as input <code> labelInstruction </code>.
 *
 * The input label has to be of type IRLABEL, otherwise NULL is returned.
 *
 * @param method IR method to consider that include <<code> labelInstruction </code>
 * @param labelInstruction Label instruction that is the target of the new branch instruction returned
 * @return A new IRBRANCH instruciton to the label given as input
 */
ir_instruction_t * IRMETHOD_newBranchToLabel (ir_method_t *method, ir_instruction_t *labelInstruction);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingBranchInstructions
 * @brief Add a new branch to the label given as input
 *
 * Add a new branch instruction (i.e., IRBRANCH) to the label given as input before the instruction <code> beforeInstruction </code>.
 *
 * The input label has to be of type IRLABEL, otherwise NULL is returned.
 *
 * @param method IR method to consider that include <<code> labelInstruction </code>
 * @param labelInstruction Label instruction that is the target of the new branch instruction returned
 * @param beforeInstruction The returned instruction is stored just before this one
 * @return A new IRBRANCH instruciton to the label given as input
 */
ir_instruction_t * IRMETHOD_newBranchToLabelBefore (ir_method_t *method, ir_instruction_t *labelInstruction, ir_instruction_t *beforeInstruction);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingBranchInstructions
 * @brief Add a new branch to the label given as input
 *
 * Add a new branch instruction (i.e., IRBRANCH) to the label given as input after the instruction <code> afterInstruction </code>.
 *
 * The input label has to be of type IRLABEL, otherwise NULL is returned.
 *
 * @param method IR method to consider that include <<code> labelInstruction </code>
 * @param labelInstruction Label instruction that is the target of the new branch instruction returned
 * @param afterInstruction The returned instruction is stored just after this one
 * @return A new IRBRANCH instruciton to the label given as input
 */
ir_instruction_t * IRMETHOD_newBranchToLabelAfter (ir_method_t *method, ir_instruction_t *labelInstruction, ir_instruction_t *afterInstruction);

/**
 * \ingroup IRMETHOD_ModifyVariables
 * @brief Get a new variable ID
 *
 * Return a free variable ID to use.
 *
 * This function can be used to create new IR variables to use.
 *
 * For example, the following code allocates a new integer 32 bits IR variable to use to store the result of an operation:
 * @code
 * ir_item_t *resultInst;
 * resultInst = IRMETHOD_getInstructionResult(inst);
 * (resultInst->value).v = IRMETHOD_newVariableID(method);
 * resultInst->type = IROFFSET;
 * resultInst->internal_type = IRINT32;
 * @endcode
 *
 * The same effect can be achieved by calling the function IRMETHOD_setInstructionParameterWithANewVariable as following:
 * @code
 * IRMETHOD_setInstructionParameterWithANewVariable(method, inst, IRINT32, NULL, 0);
 * @endcode
 *
 * @param method IR method to consider
 * @return A new variable  ID not yet used by the IR method given as input
 */
IR_ITEM_VALUE IRMETHOD_newVariableID (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyVariables
 * @brief Allocate a new variable
 *
 * Allocate a new variable and stores its information in @c newVariable.
 *
 * For example, the following code allocates a new integer 32 bits IR variable to use to store the result of an operation:
 * @code
 * ir_item_t *resultInst;
 * resultInst = IRMETHOD_getInstructionResult(inst);
 * IRMETHOD_newVariable(method, resultInst, IRINT32, NULL);
 * @endcode
 *
 * The same effect can be achieved by calling the function IRMETHOD_setInstructionParameterWithANewVariable as following:
 * @code
 * IRMETHOD_setInstructionParameterWithANewVariable(method, inst, IRINT32, NULL, 0);
 * @endcode
 *
 * @param method IR method to consider
 * @return A new variable  ID not yet used by the IR method given as input
 */
void IRMETHOD_newVariable (ir_method_t *method, ir_item_t *newVariable, JITUINT16 internal_type, TypeDescriptor *typeInfos);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the definitions of the variable given as input
 *
 * Return the list of IR instructions that define \c varID within the method \c method.
 *
 * The caller of this function is in charge to free the returned list.
 *
 * For example:
 * @code
 *
 * //Fetch all definitions of variable 5
 * XanList *defs = IRMETHOD_getVariableDefinitions(method, 5);
 *
 * // Use the definitions
 * ...
 *
 * // Free the memory
 * xanList_destroyList(defs);
 * @endcode
 *
 * @param method IR method where the returned definitions belongs to
 * @param varID Variable to consider
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getVariableDefinitions (ir_method_t *method, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get all definitions of all variables
 *
 * Return all definitions of all variables used in method @c method.
 *
 * The caller of this function is in charge to free the returned table by using the following code:
 * @code
 * XanHashTableItem *item;
 * XanHashTable *allDefs;
 * XanList *defsOfVar5;
 *
 * // Fetch all definitions of all variable
 * allDefs = IRMETHOD_getAllVariableDefinitions(method);
 *
 * // Fetch all definitions of variable 5
 * defsOfVar5 = xanHashTable_lookup(allDefs, (void *)(JITNUINT)(5 + 1));
 * if (defsOfVar5 != NULL){
 * 	XanListItem	*i;
 *	i	= xanList_first(defsOfVar5);
 *	while (i != NULL){
 *		ir_instruction_t	*def;
 *		def = i->data;
 *		printf("Instruction %u defines variable 5\n", IRMETHOD_getInstructionPosition(def));
 *		i = i->next;
 *	}
 * } else {
 * 	printf("There is no definitions of variable 5\n");
 * }
 *
 * // Free the memory
 * item = xanHashTable_first(allDefs);
 * while (item != NULL){
 *	XanList	*defs;
 *	defs	= item->element;
 * 	xanList_destroyList(defs);
 * 	item = xanHashTable_next(allDefs, item);
 * }
 * xanHashTable_destroyTable(allDefs);
 * @endcode
 *
 * @param method IR method where the returned definitions belongs to
 * @return All definitions of all variables of @c method
 */
XanHashTable * IRMETHOD_getAllVariableDefinitions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the definitions of the variable given as input that belong to a set of instructions
 *
 * Return the list of IR instructions that define \c varID within the method \c method  that belongs to the instruction set \c instructions .
 *
 * The caller of this function has to free the list when he does not need it anymore.
 *
 * To call this function, you have to run the variable liveness analysis first.
 *
 * @param method IR method where the returned definitions belongs to
 * @param varID Variable to consider
 * @param instructions Instructions to consider. List of (ir_instruction_t *).
 * @return Subset of \c instructions . List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getVariableDefinitionsWithinInstructions (ir_method_t *method, IR_ITEM_VALUE varID, XanList *instructions);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the definitions of the variable given as input that reaches a given point in the CFG
 *
 * Return the list of IR instructions that define \c varID within the method \c method that reach the instruction \c inst.
 *
 * The caller of this function has to free the list when he does not need it anymore.
 *
 * To call this function, you have to run the reaching definition analysis first (\ref REACHING_DEFINITIONS_ANALYZER).
 *
 * @param method IR method where the returned definitions belongs to
 * @param varID Variable to consider
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getVariableDefinitionsThatReachInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the uses of the variable given as input
 *
 * Return the list of IR instructions that use \c varID within the method \c method.
 *
 * The caller of this function has to free the list when he does not need it anymore.
 *
 * To call this function, you have to run the variable liveness analysis first.
 *
 * @param method IR method where the returned instructions belongs to
 * @param varID Variable to consider
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getVariableUses (ir_method_t *method, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the uses of the variable given as input that belong to a set of instructions
 *
 * Return the list of IR instructions that use \c varID within the method \c method that belongs to the instruction set \c instructions .
 *
 * The caller of this function has to free the list when he does not need it anymore.
 *
 * To call this function, you have to run the variable liveness analysis first.
 *
 * @param method IR method where the returned instructions belongs to
 * @param varID Variable to consider
 * @param instructions Instructions to consider. List of (ir_instruction_t *).
 * @return Subset of \c instructions . List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getVariableUsesWithinInstructions (ir_method_t *method, IR_ITEM_VALUE varID, XanList *instructions);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction to an IR method
 *
 * Allocate a new IRCALL instruction, which will call the IR method specified in input with the return and parameters specified.
 *
 * @param caller IR method where the new instruction will be inserted
 * @param callee IR method that will be called at run time.
 * @param returnItem IR return type of the IR method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newCallInstruction (ir_method_t *caller, ir_method_t *callee, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction to an IR method before the one given as input
 *
 * Allocate a new IRCALL instruction, which will call the IR method specified in input with the return and parameters specified.
 *
 * The new instruction will be placed just before the instruction <code> prev </code>.
 *
 * This is the only difference with the function \ref IRMETHOD_newCallInstruction
 *
 * @param caller IR method where the new instruction will be inserted
 * @param callee IR method that will be called at run time.
 * @param prev IR instruction stored just after the one returned
 * @param returnItem IR return type of the IR method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newCallInstructionBefore (ir_method_t *caller, ir_method_t *callee, ir_instruction_t *prev, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction to an IR method after the one given as input
 *
 * Allocate a new IRCALL instruction, which will call the IR method specified in input with the return and parameters specified.
 *
 * The new instruction will be placed just after the instruction <code> prev </code>.
 *
 * This is the only difference with the function \ref IRMETHOD_newCallInstruction
 *
 * @param caller IR method where the new instruction will be inserted
 * @param callee IR method that will be called at run time.
 * @param prev IR instruction stored just before the one returned
 * @param returnItem IR return type of the IR method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newCallInstructionAfter (ir_method_t *caller, ir_method_t *callee, ir_instruction_t *prev, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction to a C method
 *
 * Allocate a new IRNATIVECALL instruction, which will call the C method specified in input with the return and parameters specified.
 *
 * Consider the case that we want to call a C function called <code> my_C_function </code>.
 *
 * The code we need to add to the codetool is the following:
 * @code
 * JITFLOAT32 my_C_function (JITINT32 runtimeValue){
 *   printf("This code will be executed at run time %d\n", runtimeValue);
 *   return 0.5;
 * }
 *
 * void dummy_do_job (ir_method_t * method){
 *   ir_item_t item;
 *   ir_item_t returnVar;
 *   XanList *params;
 *
 *   // Declare the parameters of the call
 *   params = xanList_new(allocFunction, freeFunction, NULL);
 *   memset(&item, 0, sizeof(ir_item_t));
 *   item.value.v = 10;
 *   item.type = IRINT32;
 *   item.internal_type = item.type;
 *   xanList_append(params, &item);
 *
 *   // The return value of the call will be stored to a new floating point variable
 *   IRMETHOD_newVariable(method, &returnVar, IRFLOAT32, NULL);
 *
 *   // Add the call instruction
 *   IRMETHOD_newNativeCallInstruction(method, "my_C_function", my_C_function, &returnType, params);
 *
 *   // Free the memory
 *   xanList_destroyList(params);
 * }
 * @endcode
 *
 * Consider the next my_C_function to call:
 * @code
 * void my_C_function (void){
 *   printf("This code will be executed at run time\n");
 *   return ;
 * }
 * @endcode
 * In this case, the code to add to the codetool is the following:
 * @code
 * void dummy_do_job (ir_method_t * method){
 *   IRMETHOD_newNativeCallInstruction(method, "my_C_function", my_C_function, NULL, NULL);
 * }
 * @endcode
 *
 * @param method IR method where the new instruction will be inserted
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C method to call
 * @param returnItem IR return type of the C method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newNativeCallInstruction (ir_method_t *method, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Set an existing instruction to be a native call
 *
 * Set the existing instruction @c i to be a native call to @c functionToCallPointer .
 *
 * @param method IR method where the instruction @c i belongs to.
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C method to call
 * @param returnItem IR return type of the C method that the instruction @c will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 */
void IRMETHOD_setInstructionAsNativeCall(ir_method_t *method, ir_instruction_t *i, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction before the one given as input to a C method
 *
 * Allocate a new IRNATIVECALL instruction, which will call the C method specified in input with the return and parameters specified.
 *
 * The new instruction will be placed just before the instruction <code> prev </code>.
 *
 * This is the only difference with the function \ref IRMETHOD_newNativeCallInstruction
 *
 * @param method IR method that includes the instruction <code> prev </code>
 * @param prev IR instruction stored just after the one returned
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C methdo to call
 * @param returnItem IR return type of the C method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newNativeCallInstructionBefore (ir_method_t *method, ir_instruction_t *prev, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction before the one given as input to a C method
 *
 * Allocate a new \ref IRNATIVECALL instruction, which will call the C method specified in input with the return and parameters specified.
 *
 * The new instruction will be placed just after the instruction <code> prev </code>.
 *
 * This is the only difference with the function \ref IRMETHOD_newNativeCallInstruction .
 *
 * For example, consider that we want to add a call to the following function in the IR code:
 * @code
 * JITFLOAT32 my_C_function (JITINT32 runtimeValue){
 *   printf("This code will be executed at run time %d\n", runtimeValue);
 *   return 0.5;
 * }
 * @endcode
 *
 * In this case, the following code add the call to it:
 * @code
 * void example_of_usage (ir_method_t *method, ir_instruction_t *afterInst){
 *   ir_instruction_t *callInst;
 *   ir_item_t returnVar;
 *   ir_item_t item;
 *
 *   // The return value of the call will be stored to a new floating point variable
 *   IRMETHOD_newVariable(method, &returnVar, IRFLOAT32, NULL);
 *
 *   // Add the call instruction
 *   callInst	= IRMETHOD_newNativeCallInstructionByUsingSymbolsAfter(method, afterInst, "name_of_my_function", my_C_function, &returnVar, NULL);
 *
 *   // Declare the parameters of the call
 *   memset(&item, 0, sizeof(ir_item_t));
 *   item.value.v = 10;
 *   item.type = IRINT32;
 *   item.internal_type = item.type;
 *   IRMETHOD_addInstructionCallParameter(method, callInst, &item);
 * }
 * @endcode
 *
 * @param method IR method that includes the instruction <code> prev </code>
 * @param prev IR instruction stored just before the one returned
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C methdo to call
 * @param returnItem IR return type of the C method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newNativeCallInstructionAfter (ir_method_t *method, ir_instruction_t *prev, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction before the one given as input to a C method
 *
 * Allocate a new IRNATIVECALL instruction, which will call the C method specified in input with the return and parameters specified.
 *
 * The new instruction will be placed just after the instruction <code> prev </code>.
 *
 * The only difference between this function and \ref IRMETHOD_newNativeCallInstructionBefore is that this one uses symbols to specify both the name of the C function to call and its pointer.
 *
 * For example, consider that we want to add a call to the following function in the IR code:
 * @code
 * JITFLOAT32 my_C_function (JITINT32 runtimeValue){
 *   printf("This code will be executed at run time %d\n", runtimeValue);
 *   return 0.5;
 * }
 * @endcode
 *
 * In this case, the following code add the call to it:
 * @code
 * void example_of_usage (ir_method_t * method, ir_instruction_t *beforeInst, ir_symbol_t *symbolOfMyCFunction){
 *   ir_item_t item;
 *   ir_item_t returnVar;
 *   ir_instruction_t *callInst;
 *
 *   // The return value of the call will be stored to a new floating point variable
 *   IRMETHOD_newVariable(method, &returnVar, IRFLOAT32, NULL);
 *
 *   // Add the call instruction
 *   callInst	= IRMETHOD_newNativeCallInstructionByUsingSymbolsBefore(method, beforeInst, NULL, symbolOfMyCFunction, &returnVar, NULL);
 *
 *   // Declare the parameters of the call
 *   memset(&item, 0, sizeof(ir_item_t));
 *   item.value.v = 10;
 *   item.type = IRINT32;
 *   item.internal_type = item.type;
 *   IRMETHOD_addInstructionCallParameter(method, callInst, &item);
 * }
 * @endcode
 *
 * @param method IR method that includes the instruction <code> prev </code>
 * @param prev IR instruction stored just before the one returned
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C methdo to call
 * @param returnItem IR return type of the C method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newNativeCallInstructionByUsingSymbolsBefore (ir_method_t *method, ir_instruction_t *prev, ir_symbol_t *functionToCallName, ir_symbol_t *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction before the one given as input to a C method
 *
 * Allocate a new IRNATIVECALL instruction, which will call the C method specified in input with the return and parameters specified.
 *
 * The new instruction will be placed just after the instruction <code> prev </code>.
 *
 * The only difference between this function and \ref IRMETHOD_newNativeCallInstructionAfter is that this one uses symbols to specify both the name of the C function to call and its pointer.
 *
 * @param method IR method that includes the instruction <code> prev </code>
 * @param prev IR instruction stored just before the one returned
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C methdo to call
 * @param returnItem IR return type of the C method that the new instruction will call at run time. The content of this parameter will be copied without linking to it.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The content of these parameters will be copied without linking to or freeing them.
 * @return new IR instruction
 */
ir_instruction_t * IRMETHOD_newNativeCallInstructionByUsingSymbolsAfter (ir_method_t *method, ir_instruction_t *prev, ir_symbol_t *functionToCallName, ir_symbol_t *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Clone an IR instruction
 *
 * Clone the IR instruction <code> instToClone </code> and store it at the end of the method.
 * Notice that <code> instToClone </code> can be part of a method different that <code> method </code>.
 *
 * @param method IR method where the returned instruction is inserted
 * @param instToClone IR instruction to clone and insert in <code> method </code>
 * @return clone of the instruction <code> instToClone </code>
 */
ir_instruction_t * IRMETHOD_cloneInstruction (ir_method_t *method, ir_instruction_t *instToClone);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Clone an IR instruction
 *
 * Clone the IR instruction <code> instToClone </code> and insert it just before <code> beforeInst </code>.
 *
 * Notice that <code> instToClone </code> can be part of a method different that <code> method </code>.
 *
 * @param method IR method where the returned instruction is inserted
 * @param instToClone IR instruction to clone and insert in <code> method </code>
 * @param beforeInst Instruction stored just after the returned one.
 * @return clone of the instruction <code> instToClone </code>
 */
ir_instruction_t * IRMETHOD_cloneInstructionAndInsertBefore (ir_method_t *method, ir_instruction_t *instToClone, ir_instruction_t *beforeInst);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Clone an IR instruction
 *
 * Clone the IR instruction <code> instToClone </code> and insert it just after <code> afterInst </code>.
 *
 * Notice that <code> instToClone </code> can be part of a method different that <code> method </code>.
 *
 * @param method IR method where the returned instruction is inserted
 * @param instToClone IR instruction to clone and insert in <code> method </code>
 * @param afterInst Instruction stored just after the returned one.
 * @return clone of the instruction <code> instToClone </code>
 */
ir_instruction_t * IRMETHOD_cloneInstructionAndInsertAfter (ir_method_t *method, ir_instruction_t *instToClone, ir_instruction_t *afterInst);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the signature of a IR method
 *
 * Return the signature of the IR method given as input.
 * The returned structure is part of the IR method \c method, which means the caller cannot free it.
 *
 * @param method IR method to consider
 * @return IR signature of the input method
 */
ir_signature_t * IRMETHOD_getSignature (ir_method_t * method);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction is a jump
 *
 * Return JITTRUE if the instruction \c inst is a jump instruction, which means that the program counter changes not in the normal way after its execution.
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_mayHaveNotFallThroughInstructionAsSuccessor (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check whether an instruction is a jump.
 *
 * Return JITTRUE if \c inst is a jump instruction.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAJumpInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type performs a jump operation
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a jump operation.
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAJumpInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check whether an instruction is a branch.
 *
 * Return JITTRUE if \c inst is a branch instruction.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isABranchInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check whether an instruction of the input type performs a branch.
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a branch.
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isABranchInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction handles exceptions
 *
 * Return JITTRUE if the instruction <code> inst </code> handles exceptions.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAnExceptionInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type handles exceptions
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> handles exceptions.
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAnExceptionInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction performs a mathematic operation
 *
 * Return JITTRUE if the instruction <code> inst </code> performs a mathematic operation.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAMathInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type performs a mathematic operation
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a mathematic operation.
 *
 * Return JITFALSE otherwise;
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAMathInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction performs a heap memory management operation
 *
 * Return JITTRUE if the instruction <code> inst </code> performs a heap memory management operation.
 *
 * Return JITFALSE otherwise;
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAMemoryAllocationInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type performs a heap memory management operation
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a heap memory management operation.
 *
 * Return JITFALSE otherwise;
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAMemoryAllocationInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction performs a load/store to memory
 *
 * Return JITTRUE if the instruction <code> inst </code> performs a load (store) from (to) memory (this function does not consider memory allocation/deallocation instructions like \ref IRALLOC as memory operations).
 *
 * Return JITFALSE otherwise;
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAMemoryInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type performs a compare operation
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a load (store) from (to) memory.
 *
 * Return JITFALSE otherwise;
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAMemoryInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction performs a type conversion
 *
 * Return JITTRUE if the instruction <code> inst </code> performs a type conversion.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAConversionInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type performs a type conversion
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a type conversion.
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAConversionInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction performs a compare operation
 *
 * Return JITTRUE if the instruction <code> inst </code> performs a compare operation.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isACompareInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type performs a compare operation
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a compare operation.
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isACompareInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if the input instruction performs a bitwise operation
 *
 * Return JITTRUE if the instruction <code> inst </code> performs a bitwise operation.
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isABitwiseInstruction (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction of the input type performs a bitwise operation
 *
 * Return JITTRUE if an instruction of type <code> instructionType </code> performs a bitwise operation.
 *
 * @param instructionType IR instruction type to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isABitwiseInstructionType (JITUINT16 instructionType);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction uses escaped variables
 *
 * Return JITTRUE if the instruction \c inst uses or defines an escaped variable; JITFALSE otherwise.
 * Both liveness of variables and escapes information has to be computed before calling this function.
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasInstructionEscapes (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction may access to heap
 *
 * Return JITTRUE if the instruction \c inst may access to already allocated heap memory at runtime; JITFALSE otherwise.
 * Both liveness of variables and escapes information has to be computed before calling this function.
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_mayInstructionAccessHeapMemory (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the name of the method
 *
 * Return the name of the method.
 * The return value can be used as a string pointer of the C language (i.e., char *) and it cannot be freed by the caller.
 * The name does not include the namespace of the method.
 *
 * @param method IR method to consider
 * @return Method name
 */
JITINT8 * IRMETHOD_getMethodName (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModfyMethods
 * @brief Get the name of the bytecode class
 *
 * Return the name of the bytecode class given as input.
 * The return value can be used as a string pointer of the C language (i.e., char *) and it cannot be freed by the caller.
 *
 * @param classDescriptor Bytecode class
 * @return The class name as string
 */
JITINT8 * IRMETHOD_getClassName (TypeDescriptor * classDescriptor);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the name including the name space of the method
 *
 * Return the name of the method appended to the name of the class that contains the method.
 * The return value can be used as a string pointer of the C language (i.e., char *) and it cannot be freed by the caller.
 *
 * @param method IR method to consider
 * @return Method name containing the name space
 */
JITINT8 * IRMETHOD_getCompleteMethodName (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the name of the C native function that implements the method
 *
 * If the body of the method is provided by a native C function rather than by code generated by ILDJIT, then this function returns the name of this C native function.
 *
 * Otherwise, NULL is returned.
 *
 * @param method IR method to consider
 * @return Name of the C function
 */
JITINT8 * IRMETHOD_getNativeMethodName (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the method signature as string
 *
 * Return the signature of self method in a string format.
 * The return value can be used as a string pointer of the C language (i.e., char *) and it cannot be freed by the caller.
 *
 * @param method IR method to consider
 * @return The signature of the method in string format
 */
JITINT8 * IRMETHOD_getSignatureInString (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the label instruction that defines \c labelID
 *
 * Return the IR instruction of the method which sets the label \c labelID
 *
 * @param method IR method where the instruction returned belongs to
 * @param labelID label ID that the returned instruction defines
 * @return IR instruction that defines the label \c labelID
 */
ir_instruction_t * IRMETHOD_getLabelInstruction (ir_method_t *method, IR_ITEM_VALUE labelID);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the filter instruction that defines \c labelID
 *
 * Return the IR instruction of the method which sets the label \c labelID
 *
 * @param method IR method where the instruction returned belongs to
 * @param labelID label ID that the returned instruction defines
 * @return IR instruction that defines the label \c labelID
 */
ir_instruction_t * IRMETHOD_getFilterInstruction (ir_method_t *method, IR_ITEM_VALUE labelID);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the finally instruction that defines \c labelID
 *
 * Return the IR instruction of the method which sets the label \c labelID
 *
 * @param method IR method where the instruction returned belongs to
 * @param labelID label ID that the returned instruction defines
 * @return IR instruction that defines the label \c labelID
 */
ir_instruction_t * IRMETHOD_getFinallyInstruction (ir_method_t *method, IR_ITEM_VALUE labelID);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the possible callers of a finally block
 *
 * Return the list of instructions of type <code> IRCALLFINALLY </code> that jump to the finally block identified by the label <code> finallyLabelID </code>.
 *
 * A finally block begins with an instruction IRSTARTFINALLY, which identifies the specific finally block that is starting by its first parameter (in our case  <code> finallyLabelID </code>).
 *
 * @param method IR method where the instructions returned belong to
 * @param finallyLabelID label ID of the beginning of the finally block
 * @return IR instructions that call the finally block
 */
XanList * IRMETHOD_getCallersOfFinallyBlock (ir_method_t *method, IR_ITEM_VALUE finallyLabelID);

/**
 * \ingroup IRMETHOD_ModifyInstructions
 * @brief Reset the fields of an instruction
 *
 * Erase to the initial values each fields of the instruction \c inst given as input.
 *
 * @param method IR method that includes the instruction given as input
 * @param inst IR instruction to reset
 */
void IRMETHOD_eraseInstructionFields (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the instruction position
 *
 * Return the position of the method where the instruction \c inst has been stored.
 * Pay attention on the fact that if some instruction has been inserted, deleted, moved, then the position of each other instruction will be changed potentially.
 * For this reason, if you need a real identificator of an instruction, then please use its memory address.
 *
 * @param inst IR instruction to consider
 * @return position within the IR method of \c inst where it is stored
 */
JITUINT32 IRMETHOD_getInstructionPosition (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ModifyMethodsDeletingInstructions
 * @brief Remove one instruction
 *
 * Remove and free the instruction given as input from the IR method \c method.
 *
 * @param method IR method where \c inst belongs to
 * @param inst IR instruction to remove
 */
void IRMETHOD_deleteInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ModifyMethodsDeletingInstructions
 * @brief Remove every instructions of a method
 *
 * Remove and free every instruction of the method given as input.
 *
 * Nop instructions at the beginning of the method are destroyed as well.
 * Hence, you may want to invoke \ref IRMETHOD_addMethodDummyInstructions before refilling the method @c method after this call.
 *
 * @param method IR method to consider
 */
void IRMETHOD_deleteInstructions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the instruction at a given position
 *
 * Return the instruction stored inside the position \c position given as input.
 *
 * @param method IR method where the returned instruction belongs to
 * @param position Position where the returned instruction is stored
 * @return IR instruction stored within the specified position
 */
ir_instruction_t * IRMETHOD_getInstructionAtPosition (ir_method_t *method, JITUINT32 position);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the positions of all instructions of a given IR method
 *
 * Return the positions of all instructions of @c method at the time the call is performed.
 *
 * The returned table is the mapping from the instruction pointer to the position number plus 1.
 *
 * The returned table can be used to cache the positions of instructions before a given transformation. For example:
 * @code
 * XanHashTable *t;
 * t = IRMETHOD_getInstructionPositions(method);
 * // Code analysis
 * ...
 *
 * // Code transformation that can affect instruction positions
 * ...
 *
 * // Code that need instructions position before the transformation
 * JITUINT32 previousPosition;
 * previousPosition = ((JITUINT32)(JITNUINT)xanHashTable_lookup(t, inst)) - 1;
 * @endcode
 *
 * The caller is in charge of freeing the returned table by calling the following function:
 * @code
 * xanHashTable_destroyTable(t);
 * @endcode
 *
 * @param method IR method where the returned instruction belongs to
 * @return First IR instruction of the method
 */
XanHashTable * IRMETHOD_getInstructionPositions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the first instruction of a IR method
 *
 * Return the first IR instruction of the method that will be executed if \c method will be called
 *
 * @param method IR method where the returned instruction belongs to
 * @return First IR instruction of the method
 */
ir_instruction_t * IRMETHOD_getFirstInstruction (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the first instruction of a given type of a IR method
 *
 * Return the first IR instruction of type @c type that belongs to the method @c method .
 *
 * @param method IR method where the returned instruction belongs to
 * @param type Type of the instruction returned
 * @return First IR instruction of the method
 */
ir_instruction_t * IRMETHOD_getFirstInstructionOfType (ir_method_t *method, JITUINT16 type);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the instruction stored just next to the one given as input
 *
 * Return the instruction stored just next to the one given as input
 *
 * @param method IR method that includes \c prev
 * @param inst IR instruction to consider
 * @return The next IR instruction
 */
ir_instruction_t * IRMETHOD_getNextInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the first instruction stored next to the one given as input which is not of a given type
 *
 * Return the first instruction stored next to the one given as input which is not of type \c type .
 *
 * @param method IR method that includes \c prev
 * @param prev IR instruction to consider
 * @param type Instruction type to skip
 * @return The next IR instruction
 */
ir_instruction_t * IRMETHOD_getNextInstructionNotOfType (ir_method_t *method, ir_instruction_t *prev, JITUINT16 type);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the instruction stored just before to the one given as input
 *
 * Return the instruction stored just before to the one given as input
 *
 * @param method IR method that includes \c prev
 * @param inst IR instruction to consider
 * @return The next IR instruction
 */
ir_instruction_t * IRMETHOD_getPrevInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the first instruction of a IR method
 *
 * Return the first IR instruction of the method that will be executed if \c method will be called
 *
 * @param method IR method where the returned instruction belongs to
 * @param type Instruction type to skip
 * @return First IR instruction of the method
 */
ir_instruction_t * IRMETHOD_getFirstInstructionNotOfType (ir_method_t *method, JITUINT16 type);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Get the last instruction of a IR method
 *
 * Return the IR instruction of the method stored at the bottom of the method.
 *
 * @param method IR method where the returned instruction belongs to
 * @return Last IR instruction of the method
 */
ir_instruction_t * IRMETHOD_getLastInstruction (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the list of instructions that compose a method
 *
 * Return the list of instructions that compose the method <code> method </code>.
 *
 * The caller is in charge of freeing the returned list by calling the \ref xanList_destroyList function of the XanList data structure.
 * @code
 * XanList *l;
 * l = IRMETHOD_getInstructions(method);
 * ...
 * xanList_destroyList(l);
 * @endcode
 * Every element of the list is a pointer to the data structure <code> ir_instruction_t </code>.
 * The memory pointed by these addresses contains the up-to-date information of the instructions (i.e., changing the instructions of the returned list will change the instructions of the method as well).
 *
 * @param method IR method that contains the returned list of instructions
 * @return List of instructions (ir_instruction_t *)
 */
XanList * IRMETHOD_getInstructions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_InstructionSequence
 * @brief Return the set of instructions that compose a method
 *
 * Return the set of <instruction, position> pairs that compose the method <code> method </code>.
 *
 * The index of the returned array is the position of the instruction pointed by the pointer stored in that array element.
 *
 * The caller is in charge of freeing the returned list by calling \ref freeFunction .
 * @code
 * ir_instruction_t **insns;
 *
 * // Fetch all instructions
 * insns = IRMETHOD_getInstructions(method);
 *
 * // Print instructions
 * for (JITUINT32 i=0; i < IRMETHOD_getInstructionsNumber(method); i++){
 *      ir_instruction_t    *insn;
 *
 *      // Fetch the instruction stored at position i
 *      insn    = insns[i];
 *
 *      printf("Instruction stored at position %u: %s\n", i, IRMETHOD_getInstructionTypeName(insn->type));
 * }
 *
 * // Free the memory
 * freeFunction(insns);
 * @endcode
 * Every element of the returned array is a pointer to the data structure <code> ir_instruction_t </code>.
 * The memory pointed by these addresses contains the up-to-date information of the instructions (i.e., changing the instructions of the returned list will change the instructions of the method as well).
 *
 * @param method IR method that contains the returned list of instructions
 * @return Instructions that compose @c method
 */
ir_instruction_t ** IRMETHOD_getInstructionsWithPositions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the list of labels used inside a method
 *
 * Return the list of instructions of type IRLABEL that compose the method <code> method </code>.
 *
 * This function is equivalent of
 * @code
 * IRMETHOD_getInstructionsOfType(method, IRLABEL);
 * @endcode
 *
 * The caller is in charge of freeing the returned list by calling the \ref xanList_destroyList function of the XanList data structure.
 * @code
 * XanList *l;
 * l = IRMETHOD_getInstructions(method);
 * ...
 * xanList_destroyList(l);
 * @endcode
 * Every element of the list is a pointer to the data structure <code> ir_instruction_t </code>.
 *
 * The memory pointed by these addresses contains the up-to-date information of the instructions (i.e., changing the instructions of the returned list will change the instructions of the method as well).
 *
 * @param method IR method that contains the returned list of instructions
 * @return List of instructions (ir_instruction_t *)
 */
XanList * IRMETHOD_getLabelInstructions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the list of memory instructions of a method
 *
 * Return the list of memory instructions that compose the method <code> method </code>.
 *
 * An instruction is a memory one based on the following function:
 * @code
 * IRMETHOD_isAMemoryInstruction
 * @endcode
 *
 * The caller is in charge of freeing the returned list by calling the \ref xanList_destroyList function of the XanList data structure.
 * @code
 * XanList *l;
 *
 * // Fetch the set of memory instructions of a method
 * l = IRMETHOD_getMemoryInstructions(method);
 *
 * // Use the list
 * ...
 *
 * // Free the memory
 * xanList_destroyList(l);
 * @endcode
 * Every element of the list is a pointer to the data structure <code> ir_instruction_t </code>.
 *
 * The memory pointed by these addresses contains the up-to-date information of the instructions (i.e., changing the instructions of the returned list will change the instructions of the method as well).
 *
 * @param method IR method that contains the returned list of instructions
 * @return List of instructions (ir_instruction_t *)
 */
XanList * IRMETHOD_getMemoryInstructions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the list of call instructions of a method
 *
 * Return the list of call instructions that compose the method <code> method </code>.
 *
 * An instruction is a call one if the following function returns JITTRUE:
 * @code
 * IRMETHOD_isACallInstruction
 * @endcode
 *
 * The caller is in charge of freeing the returned list by calling the \ref xanList_destroyList function of the XanList data structure.
 * @code
 * XanList *l;
 *
 * // Fetch the set of call instructions of a method
 * l = IRMETHOD_getCallInstructions(method);
 *
 * // Use the list
 * ...
 *
 * // Free the memory
 * xanList_destroyList(l);
 * @endcode
 * Every element of the list is a pointer to the data structure <code> ir_instruction_t </code>.
 *
 * The memory pointed by these addresses contains the up-to-date information of the instructions (i.e., changing the instructions of the returned list will change the instructions of the method as well).
 *
 * @param method IR method that contains the returned list of instructions
 * @return List of instructions (ir_instruction_t *)
 */
XanList * IRMETHOD_getCallInstructions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Return the list of instructions of a specific type inside a method
 *
 * Return the list of instructions of type <code> instructionsType </code> that compose the method <code> method </code>.
 *
 * This function is equivalent of
 * @code
 * IRMETHOD_getInstructionsOfType(method, IRLABEL);
 * @endcode
 *
 * The caller is in charge of freeing the returned list by calling the \ref xanList_destroyList function of the XanList data structure.
 * @code
 * XanList *l;
 *
 * // Fetch the set of labels
 * l = IRMETHOD_getInstructionsOfType(method, IRLABEL);
 *
 * // Use the elements in the list
 * item = xanList_first(l);
 * while (item != NULL){
 *   ir_instruction_t *i;
 *   i = item->data;
 *   ...
 *   item = item->next;
 * }
 *
 * // Free the memory
 * xanList_destroyList(l);
 * @endcode
 * Every element of the list is a pointer to the data structure <code> ir_instruction_t </code>.
 *
 * The memory pointed by these addresses contains the up-to-date information of the instructions (i.e., changing the instructions of the returned list will change the instructions of the method as well).
 *
 * @param method IR method that contains the returned list of instructions.
 * @param instructionsType Type of all the instructions returned.
 * @return List of instructions (ir_instruction_t *)
 */
XanList * IRMETHOD_getInstructionsOfType (ir_method_t *method, JITUINT16 instructionsType);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Check whether a given instruction can be reacheed from a given position of the CFG.
 *
 * Return JITTRUE if instruction @c inst can be reached from @c position; JITFALSE otherwise.
 *
 * If @c inst can be reached from @c position, then it means that after the execution of @c position, @c inst can be executed.
 *
 * On the other hand, if @c inst cannot be reached from @c position, then it means that after executing @c position, @c inst cannot be executed.
 *
 * This function requires \ref REACHING_INSTRUCTIONS_ANALYZER .
 *
 * @param method IR method
 * @param inst Instruction that is checked if it can be reached.
 * @param position Position in the CFG
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_canInstructionBeReachedFromPosition (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *position);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Get the set of instructions that can be reached from a given position of the CFG.
 *
 * Return the set of instructions that can be reached from a given position of the CFG.
 *
 * Each instruction in the table returned, say @c inst, can be reached from @c position; hence, after the execution of @c position, @c inst can be executed.
 *
 * The caller is in charge of freeing the returned table by calling \ref xanHashTable_destroyTable .
 *
 * An example of code is the following:
 * @code
 * XanHashTable *t;
 *
 * // Fetch the set of instructions that can be reached from position
 * t = IRMETHOD_getInstructionsThatCanBeReachedFromPosition(m, position);
 *
 * // Check if a given instruction can be reached from position
 * if (xanHashTable_lookup(t, inst) != NULL){
 *    // The instruction inst can be reached from position
 *
 * } else {
 *    // The instruction inst cannot be reached from position
 * }
 *
 * // Free the memory
 * xanHashTable_destroyTable(t);
 * @endcode
 *
 * This function requires \ref REACHING_INSTRUCTIONS_ANALYZER .
 *
 * @param method IR method that includes @c position and all instructions returned.
 * @param position Position in the CFG
 * @return Set of instructions reachable from @c position
 */
XanHashTable * IRMETHOD_getInstructionsThatCanBeReachedFromPosition (ir_method_t *method, ir_instruction_t *position);

/**
 * \ingroup IRMETHOD_Method_Instructions
 * @brief Get the set of instructions that can have been executed before another one.
 *
 * Return the set of instructions that can have been executed before @c inst.
 *
 * Each instruction in the table returned, say @c inst, can have been executed before a run time instance of @c inst.
 *
 * The caller is in charge of freeing the returned table by calling \ref xanHashTable_destroyTable .
 *
 * An example of code is the following:
 * @code
 * XanHashTable *t;
 *
 * // Fetch the set of instructions that can be reached from position
 * t = IRMETHOD_getInstructionsThatCanHaveBeenExecutedBeforeInstruction(m, position);
 *
 * // Check if a given instruction can be reached from position
 * if (xanHashTable_lookup(t, inst) != NULL){
 *    // The instruction inst can be reached from position
 *
 * } else {
 *    // The instruction inst cannot be reached from position
 * }
 *
 * // Free the memory
 * xanHashTable_destroyTable(t);
 * @endcode
 *
 * This function requires \ref REACHING_INSTRUCTIONS_ANALYZER .
 *
 * @param method IR method that includes @c inst and all instructions returned.
 * @param inst Instruction to consider
 * @return Set of instructions
 */
XanHashTable * IRMETHOD_getInstructionsThatCanHaveBeenExecutedBeforeInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Check escaped variable
 *
 * Return <code> JITTRUE </code> if the variable \c varID is an escaped variable within the IR method \c method.
 * Return <code> JITFALSE </code> otherwise.
 *
 * @param method IR method to consider
 * @param varID ID of the variable to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAnEscapedVariable (ir_method_t *method, JITUINT32 varID);

/**
 * \ingroup IRMETHOD_ModifyVariables
 * @brief Set variables as escaped
 *
 * Set the variable \c varID as an escaped one.
 *
 * @param method IR method to consider
 * @param varID ID of the variable to consider
 */
void IRMETHOD_setVariableAsEscaped (ir_method_t *method, JITUINT32 varID);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate the memory to store escaped variables information
 *
 * Allocate a number of processor words to store the escapes variables analysis.
 * Here each bit is reserved for a single variable (i.e. the i-th bit is the i-th variable).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of blocks of memory needed
 */
void IRMETHOD_allocEscapedVariablesMemory (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyProgram
 * @brief Allocate a new IR method
 *
 * Create a new empty IR method.
 *
 * The pointer <code> name </code> will be maintained by the library and therefore it must point to a memory location that the caller will never free.
 *
 * That memory will be freed when the method will be destroyed by calling <code> IRMETHOD_destroyMethod </code>.
 *
 * An example of usage that does not generate memory leaks is the following:
 * @code
 * ir_method_t *m;
 * char *methodName = "myNewMethod";
 *
 * // Clone the method
 * m = IRMETHOD_newMethod(strdup(methodName));
 *
 * // Use of m
 * // ...
 *
 * // Free the memory
 * IRMETHOD_destroyMethod(m);
 * @endcode
 *
 * @param name The name of the new method returned
 * @return A new empty IR method
 */
ir_method_t * IRMETHOD_newMethod (JITINT8 *name);

/**
 * \ingroup IRMETHOD_ModifyProgram
 * @brief Clone a IR method
 *
 * Clone the IR method <code> method </code> given as input.
 *
 * The pointer <code> name </code> will be maintained by the library and therefore it must point to a memory location that the caller will never free.
 *
 * That memory will be freed when the method will be destroyed by calling <code> IRMETHOD_destroyMethod </code>.
 *
 * An example of usage that does not generate memory leaks is the following:
 * @code
 * void f (ir_method_t *m){
 * 	ir_method_t *cloned_m;
 * 	char *methodName = "myNewMethod";
 *
 *	// Clone the method
 * 	cloned_m = IRMETHOD_cloneMethod(m, strdup(methodName));
 *
 * 	// Use of m
 *	//...
 *
 *	// Free the memory
 * 	IRMETHOD_destroyMethod(m);
 * }
 * @endcode

 * @param method IR method to clone
 * @param name Name of the new method returned
 * @return cloned method called @c name
 */
ir_method_t * IRMETHOD_cloneMethod (ir_method_t *method, JITINT8 *name);

/**
 * \ingroup IRMETHOD_ModifyProgram
 * @brief Destroy an IR method
 *
 * Delete the IR method <code> method </code>.
 *
 * @param method IR method to destroy
 */
void IRMETHOD_destroyMethod (ir_method_t *method);

/**
 * \ingroup IRMETHOD_CodeTransformationDataTypeGlobal
 * @brief Create a new global
 *
 * Create a new global for the program that is compiling.
 *
 * @code
 * ir_global_t *g;
 *
 * // Define the value to store into the global that is going to be created later
 * myInitialValue = allocFunction(100);
 * memset(myInitialValue, 1, 100);
 *
 * // Create a global to store 100 Bytes of constant data equal to "1"
 * g = IRGLOBAL_createGlobal("myBlobToStoreIntoDataSection", IRVALUETYPE, 100, JITTRUE, myInitialValue);
 *
 * // Free the memory
 * freeFunction(myInitialValue);
 * @endcode
 *
 * @param globalName Name of the global. This can be used to debug the dumped binary.
 * @param irType IR type of the global. Use \ref IRVALUETYPE for generic blobs.
 * @param size Number of bytes of the global
 * @param isConstant Whether the global can change value or not
 * @param initialValue Memory to read to define the initial value of the global
 * @return The new global added
 */
ir_global_t * IRGLOBAL_createGlobal (JITINT8 *globalName, JITUINT16 irType, JITUINT64 size, JITBOOLEAN isConstant, void *initialValue);

/**
 * \ingroup IRMETHOD_CodeTransformationDataTypeGlobal
 * @brief Set a global to an IR item
 *
 * Store the global @c global of type @c internal_type into the IR item @c item .
 *
 * For example:
 * @code
 *
 * // Fetch an IR item
 * ir_item_t *p = IRMETHOD_getInstructionParameter1(inst);
 *
 * // Create a global to store a pointer
 * ir_global_t *g = IRGLOBAL_createGlobal("myGlobal", IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);
 *
 * // Store the global into the IR item
 * IRGLOBAL_storeGlobalToIRItem(p, g, NULL);
 * @endcode
 *
 * @param item IR item to use to store @c global
 * @param global Global to store
 * @param typeInfos Input-language data type of the global (it can be NULL)
 */
void IRGLOBAL_storeGlobalToIRItem (ir_item_t *item, ir_global_t *global, TypeDescriptor *typeInfos);

/**
 * \ingroup IRMETHOD_CodeTransformationDataTypeGlobal
 * @brief Store a global to a parameter of an instruction.
 *
 * Store the global @c global to the specified parameter of the instruction @c inst.
 *
 * Parameter zero represents the return parameter.
 *
 * The global must be the output of the call \ref IRGLOBAL_createGlobal
 *
 * For example,
 * @code
 * ir_global_t 	*g;
 * ir_item_t 	*irItem;
 *
 * // Create the global to store
 * g = IRGLOBAL_createGlobal("MyGlobal", IRUINT32, IRDATA_getSizeOfIRType(IRUINT32), JITFALSE, NULL);
 *
 * // Store the symbol
 * IRGLOBAL_storeGlobalToInstructionParameter(m, inst, 1, g, NULL);
 * @endcode
 *
 * @param method Method that contains the parameter @c inst.
 * @param inst Instruction where the global will be stored to.
 * @param toParam Parameter of @c inst where the global will be stored to.
 * @param global Global to store
 * @param typeInfos Optional high level descriptor of the global @c global.
 */
void IRGLOBAL_storeGlobalToInstructionParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT8 toParam, ir_global_t *global, TypeDescriptor *typeInfos);

/**
 * \ingroup IRMETHOD_CodeGeneration
 * @brief Prepare an IR method to be compiled using JIT tecnique
 *
 * Prepare trampolines of the method <code> method </code>, which are necessary within the JIT compilation.
 *
 * As the function \ref IRMETHOD_translateMethodToIR, this one can be called after the IR signature is provided.
 *
 * @param method IR method to consider
 */
void IRMETHOD_createMethodTrampoline (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Free memory used for code transformation
 *
 * Free every memory used within the code optimization process (e.g. data-flow information).
 *
 * @param method IR method to consider
 */
void IRMETHOD_destroyMethodExtraMemory (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory used for code transformation
 *
 * Allocate memory used within the code optimization process (e.g. data-flow information).
 *
 * @param method IR method to consider
 */
void IRMETHOD_allocateMethodExtraMemory (ir_method_t *method);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Add a new basic block
 *
 * Allocate and add a new basic block structure information.
 * This structure can be used to store basic block information.
 * Notice that this function does not add any instruction to the IR method \c method
 *
 * @param method IR method to consider
 * @return The new basic block structure
 */
IRBasicBlock * IRMETHOD_newBasicBlock (ir_method_t *method);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get a basic block of a IR method from its position.
 *
 * Return the basic block stored inside the position given as input.
 *
 * @param method IR method where the returned basic block belongs to
 * @param position Number of the returned basic block
 * @return Basic block stored within the position \c basicBlockNumber
 */
IRBasicBlock * IRMETHOD_getBasicBlockAtPosition (ir_method_t *method, JITUINT32 position);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the position of a basic block within a IR method.
 *
 * Return the position of the basic block given as input.
 *
 * @param method IR method that \c bb belongs to
 * @param bb Basic block to get the position of
 * @return The position of \c bb within \c method
 */
JITINT32 IRMETHOD_getBasicBlockPosition(ir_method_t *method, IRBasicBlock *bb);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the first basic block of a IR method
 *
 * Return the first basic block executed within the IR method.
 *
 * @param method IR method where the returned basic block belongs to
 * @return First basic block executed within the method
 */
IRBasicBlock * IRMETHOD_getFirstBasicBlock (ir_method_t *method);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the next basic block of a IR method
 *
 * Return the next basic block of <code> bb </code> within the IR method <code> method </code>.
 *
 * Notice that there is no relation with the runtime successor of the basic block given as input.
 * The returned basic block is the one stored next to the basic block <code> bb </code>.
 *
 * @param method IR method where the returned basic block belongs to
 * @param bb Basic block to consider
 * @return Next basic block of <code> bb </code>
 */
IRBasicBlock * IRMETHOD_getNextBasicBlock (ir_method_t *method, IRBasicBlock *bb);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the next basic block of a IR method
 *
 * Return the previous basic block of <code> bb </code> within the IR method <code> method </code>.
 *
 * Notice that there is no relation with the runtime predecessor of the basic block given as input.
 * The returned basic block is the one stored previous to the basic block <code> bb </code>.
 *
 * @param method IR method where the returned basic block belongs to
 * @param bb Basic block to consider
 * @return Previous basic block of <code> bb </code>
 */
IRBasicBlock * IRMETHOD_getPrevBasicBlock (ir_method_t *method, IRBasicBlock *bb);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the number of instructions of a basic block
 *
 * Get the number of instructions of a basic block
 *
 * @param bb A basic block
 * @result Number of instructions of the basic block
 */
JITUINT32 IRMETHOD_getBasicBlockLength (IRBasicBlock *bb);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the first instruction of a basic block
 *
 * Get the first instruction of the basic block <code> bb </code> given as input
 *
 * @param method IR method where the basic block belongs to
 * @param bb Basic block to consider
 * @return First instruction of the basic block
 */
ir_instruction_t * IRMETHOD_getFirstInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the last instruction of a basic block
 *
 * Get the last instruction of the basic block <code> bb </code> given as input
 *
 * @param method IR method where the basic block belongs to
 * @param bb Basic block to consider
 * @return Last instruction of the basic block
 */
ir_instruction_t * IRMETHOD_getLastInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the next instruction of a basic block
 *
 * Get the next instruction of <code> inst </code> of the basic block <code> bb </code>.
 *
 * In case the instruction <code> inst </code> is the last one of the basic block, this function return NULL.
 *
 * In case the parameter <code> inst </code> is NULL, this function returns the first instruction of the basic block.
 *
 * @param method IR method where the basic block belongs to
 * @param bb Basic block to consider
 * @param inst Instruction to consider
 * @return Next instruction of the basic block
 */
ir_instruction_t * IRMETHOD_getNextInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the previous instruction of a basic block
 *
 * Get the previous instruction of <code> inst </code> of the basic block <code> bb </code>.
 *
 * In case the instruction <code> inst </code> is the first one of the basic block, this function return NULL.
 *
 * In case the parameter <code> inst </code> is NULL, this function returns the last instruction of the basic block.
 *
 * @param method IR method where the basic block belongs to
 * @param bb Basic block to consider
 * @param inst Instruction to consider
 * @return Previous instruction of the basic block
 */
ir_instruction_t * IRMETHOD_getPrevInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Delete basic block information of a IR method
 *
 * Free the memory used to store basic blocks information of the IR method <code> method </code>.
 *
 * @param method IR method to consider
 */
void IRMETHOD_deleteBasicBlocksInformation (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Delete circuits information of a IR method
 *
 * Free the memory used to store circuits information of the IR method <code> method </code>
 *
 * @param method IR method to consider
 */
void IRMETHOD_deleteCircuitsInformation (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get a local variable of a IR method
 *
 * Return the local variable of the IR method given as input.
 * Notice that local variables are defined by the bytecode program (e.g. CIL), which means they are the variables specified by the user within the source program (e.g. C, C#, etc...)
 *
 * @param method IR method where the local variable is defined
 * @param var_number Number of the local variable to return
 * @return A local variable of the input IR method
 */
ir_local_t * IRMETHOD_getLocalVariableOfMethod (ir_method_t * method, JITUINT32 var_number);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Insert a new local variable to the method
 *
 * Insert a new local variable to the IR method given as input.
 * This function is usually called by the translator from the input language to the IR language.
 *
 * @param method IR method to consider
 * @param internal_type IR type of the local variable
 * @param bytecodeType Bytecode type of the local variable
 * @return IR local variable inserted
 */
ir_local_t * IRMETHOD_insertLocalVariableToMethod (ir_method_t * method, JITUINT32 internal_type, TypeDescriptor * bytecodeType);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the number of local variables
 *
 * Return the number of variables of the method defined within the bytecode program (i.e., local variables).
 * Notice that local variables are defined by the bytecode program (e.g. CIL), which means they are the variables specified by the user within the source program (e.g. C, C#, etc...)
 * Moreover, local variables do not include temporany variables.
 *
 * @param method IR method to consider
 * @return Number of local variables
 */
JITUINT32 IRMETHOD_getMethodLocalsNumber (ir_method_t * method);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the number of formal parameters
 *
 * Return the number of formal parameters of the IR method given as input.\n
 * If the method \c method has the vararg property (number of actual parameters can be higher than the number of formal parameters), this function returns the minimum number of parameters that every caller has to provide.
 *
 * @param method IR method to consider
 * @return Number of formal parameter
 */
JITUINT32 IRMETHOD_getMethodParametersNumber (ir_method_t *method);

/**
 * \ingroup IRMETHOD_MethodBody
 * @brief Check if the method contains instructions
 *
 * Check if the method <code> method </code> contains at least one instruction that is not a IRNOP.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasMethodInstructions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingInstructions
 * @brief Set the number of formal parameters
 *
 * Allocate dummy instructions on top of the method. Each one is the definition of a parameter of method;
 *
 * @param method IR method to consider
 */
void IRMETHOD_addMethodDummyInstructions (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Check if a variable is a parameter
 *
 * Return JITTRUE if the variable \c varID is a parameter of the method; otherwise JITFALSE is returned.
 *
 * @param method IR method to consider
 * @param varID Variable to consider
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRMETHOD_isTheVariableAMethodParameter (ir_method_t *method, JITUINT32 varID);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Check if a variable is a declared variable
 *
 * Return JITTRUE if the variable <code> varID </code> is a declared variable of the method; otherwise JITFALSE is returned.
 *
 * @param method IR method to consider
 * @param varID Variable to consider
 * @return JITTRUE or JITFALSE
 */
JITINT32 IRMETHOD_isTheVariableAMethodDeclaredVariable (ir_method_t *method, JITUINT32 varID);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Check if a variable is a temporary
 *
 * Return JITTRUE if the variable \c varID is a temporary variable of the method; otherwise JITFALSE is returned.
 *
 * @param method IR method to consider
 * @param varID Variable to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isTheVariableATemporary (ir_method_t *method, JITUINT32 varID);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for variables liveness analysis
 *
 * Allocate a number of processor words to store the liveness analysis. Here each bit is reserved for a single variable (i.e. the i-th bit is the i-th variable).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForLivenessAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for reaching definitions analysis
 *
 * Allocate a number of processor words to store the reaching definitions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForReachingDefinitionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for reaching expressions analysis
 *
 * Allocate a number of processor words to store the reaching expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForReachingExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for available expressions analysis
 *
 * Allocate a number of processor words to store the available expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForAvailableExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for anticipated expressions analysis
 *
 * Allocate a number of processor words to store the anticipated expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForAnticipatedExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for post-ponable expressions analysis
 *
 * Allocate a number of processor words to store the postponable expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForPostponableExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for used expressions analysis
 *
 * Allocate a number of processor words to store the used expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForUsedExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for earliest expressions analysis
 *
 * Allocate a number of processor words to store the used expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForEarliestExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for latest placement analysis
 *
 * Allocate a number of processor words to store the used expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForLatestPlacementsAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Allocate memory for partial redundancy elimination analysis
 *
 * Allocate a number of processor words to store the used expressions analysis.
 * Here each bit is reserved for a single IR instruction (i.e. the i-th bit is the i-th instruction of the IR method).
 *
 * @param method IR method to consider
 * @param blocksNumber Number of memory words to allocate
 */
void IRMETHOD_allocMemoryForPartialRedundancyEliminationAnalysis (ir_method_t *method, JITUINT32 blocksNumber);

/**
 * \ingroup IRMETHOD_ModifyMethods
 * @brief Set the method descriptor
 *
 * Set the structure that describes the method by exploiting bytecode metadata.
 *
 * @param method IR method to consider
 * @param methodDescriptor Method descriptor to store within the IR method
 */
void IRMETHOD_setMethodDescriptor (ir_method_t * method, MethodDescriptor *methodDescriptor);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the number of basic blocks composing the IR method
 *
 * Return the number of basic blocks that compose the IR method.
 *
 * @param method IR method to consider
 * @return Number of basic blocks
 */
JITUINT32 IRMETHOD_getNumberOfMethodBasicBlocks (ir_method_t *method);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the basic block that includes an instruction
 *
 * Return the basic block where the instruction \c inst belongs.
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @return The basic block that includes the instruction \c inst
 */
IRBasicBlock * IRMETHOD_getBasicBlockContainingInstruction (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the basic block that includes an instruction with a specific ID.
 *
 * Return the basic block where the instruction with ID \c instID belongs.
 *
 * @param method IR method to consider
 * @param instID ID of the IR instruction to consider
 * @return The basic block that includes the instruction with ID \c instID
 */
IRBasicBlock * IRMETHOD_getBasicBlockContainingInstructionPosition (ir_method_t *method, JITUINT32 instID);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the list of instructions that compose a basic block
 *
 * Return the list of instructions that compose the basic block <code> bb </code>
 *
 * @param method IR method that includes the basic block to consider
 * @param bb Basic block to consider
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getInstructionsOfBasicBlock (ir_method_t *method, IRBasicBlock *bb);

/**
 * \ingroup IRMETHOD_BasicBlock
 * @brief Get the number of instructions of a given type within a basic block
 *
 * Get the number of instructions of a given type within a basic block
 *
 * @param method IR method that includes the basic block to consider
 * @param bb Basic block to consider
 * @param type Type of instructions to consider
 * @return Number of instructions of type <code> type </code>
 */
JITUINT32 IRMETHOD_getNumberOfInstructionsOfTypeWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb, JITUINT16 type);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Delete a call parameter of an IR instruction
 *
 * This function remove and free the memory used by the \c nth call parameter of the IR instruction \c inst
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 * @param nth Number of the call parameter to delete
 */
void IRMETHOD_deleteInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Delete every call parameter of an IR instruction
 *
 * This function remove and free the memory used by every call parameter of the IR instruction \c inst
 *
 * @param method IR method to consider
 * @param inst IR instruction to consider
 */
void IRMETHOD_deleteInstructionCallParameters (ir_method_t * method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCallParameters
 * @brief Get the number of parameters of the instruction
 *
 * This function gets the number of call parameters of the IR instruction \c inst.
 *
 * @param inst IR instruction to consider
 * @return Number of call parameters of \c inst
 */
JITUINT32 IRMETHOD_getInstructionCallParametersNumber (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCallParameters
 * @brief Get a call parameter of a IR instruction
 *
 * Return the parameter \c nth of the call.
 *
 * You can use the return structure to set/get its field. The first call parameter is 0, the second is 1, etc...
 *
 * @param inst IR instruction to consider
 * @param nth Position of the call parameter returned
 * @return Call parameter of \c inst stored within the position \c nth
 */
ir_item_t * IRMETHOD_getInstructionCallParameter (ir_instruction_t *inst, JITUINT32 nth);

/**
 * \ingroup IRMETHOD_InstructionCallParameters
 * @brief Get the type of a call parameter of an IR instruction
 *
 * Return the type of the \c nth parameter of the call @c inst.
 *
 * @param inst IR instruction to consider
 * @param nth Position of the call parameter to consider
 * @return Type of the call parameter requested
 */
JITUINT32 IRMETHOD_getInstructionCallParameterType (ir_instruction_t *inst, JITUINT32 nth);

/**
 * \ingroup IRMETHOD_InstructionCallParameters
 * @brief Get the integer value of a call parameter of an IR instruction
 *
 * Return the integer value of the \c nth parameter of the call @c inst.
 *
 * The first parameter is 0.
 *
 * @param inst IR instruction to consider
 * @param nth Position of the call parameter to consider
 * @return Integer value of the call parameter requested
 */
IR_ITEM_VALUE IRMETHOD_getInstructionCallParameterValue (ir_instruction_t *inst, JITUINT32 nth);

/**
 * \ingroup IRMETHOD_InstructionCallParameters
 * @brief Get the floating point value of a call parameter of an IR instruction
 *
 * Return the floating point value of the \c nth parameter of the call @c inst.
 *
 * The first parameter is 0.
 *
 * @param inst IR instruction to consider
 * @param nth Position of the call parameter to consider
 * @return Floating point value of the call parameter requested
 */
IR_ITEM_FVALUE IRMETHOD_getInstructionCallParameterFValue (ir_instruction_t *inst, JITUINT32 nth);

/**
 * \ingroup IRMETHOD_InstructionCallParameters
 * @brief Check if a call parameter of an instruction is a variable
 *
 * Return JITTRUE if the call parameter <code> parameterNumber </code> of the instruction <code> inst </code> is a variable; return JITFALSE otherwise.
 *
 * The first parameter is 0.
 *
 * @param inst IR instruction to consider
 * @param parameterNumber Number of the call parameter to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isInstructionCallParameterAVariable (ir_instruction_t *inst, JITUINT32 parameterNumber);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Set the integer value of a call parameter of an IR instruction
 *
 * Set the integer value of the \c nth parameter of the call @c inst.
 *
 * The first parameter is 0.
 *
 * @param method Method that includes @c inst
 * @param inst IR instruction to consider
 * @param nth Position of the call parameter to consider
 * @param valueToSet Value to use to set the @c nth call parameter
 */
void IRMETHOD_setInstructionCallParameterValue (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth, IR_ITEM_VALUE valueToSet);

/**
 * \ingroup IRMETHOD_ModifyInstructionCallParameters
 * @brief Set the floating point value of a call parameter of an IR instruction
 *
 * Set the floating point value of the \c nth parameter of the call @c inst.
 *
 * The first parameter is 0.
 *
 * @param method Method that includes @c inst
 * @param inst IR instruction to consider
 * @param nth Position of the call parameter to consider
 * @param valueToSet Floating point value to use to set the @c nth call parameter
 */
void IRMETHOD_setInstructionCallParameterFValue (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth, IR_ITEM_FVALUE valueToSet);

/**
 * \ingroup IRMETHOD_ModifyVariables
 * @brief Compute how many variables are used by a IR method
 *
 * Update the information about the highest ID used for variables inside the method.
 * This number corresponds (if there is not segmentation over IDs) to the number of variables needed by the IR method given as input.
 *
 * @param method IR method to consider
 */
void IRMETHOD_updateNumberOfVariablesNeededByMethod (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Return how many variables are used by a IR method
 *
 * Get the information about the highest ID used for variables inside the method.
 * This number corresponds (if there is not segmentation over IDs) to the number of variables needed by the IR method given as input.
 *
 * @param method IR method to consider
 * @return Number of variables used by the IR method given as input
 */
JITUINT32 IRMETHOD_getNumberOfVariablesNeededByMethod (ir_method_t *method);

/**
 * \ingroup IRMETHOD_ModifyVariables
 * @brief Set how many variables are used by a IR method
 *
 * Set the information about the highest ID used for variables inside the method.
 * This number corresponds (if there is not segmentation over IDs) to the number of variables needed by the IR method given as input.
 *
 * @param method IR method to consider
 */
void IRMETHOD_setNumberOfVariablesNeededByMethod (ir_method_t *method, JITUINT32 var_max);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an IR instruction may throw an exception
 *
 * Return JITTRUE if the instruction given as input may throw an exception at runtime
 *
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_mayThrowException (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Check if the IR instruction stored after the one given as input can be one of its successor
 *
 * Check if the IR instruction stored after the one given as input (i.e., \c inst) can be one of its successor
 *
 * @param method IR method that includes the instruction given as input
 * @param inst IR instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_canTheNextInstructionBeASuccessor (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ModifyInstructions
 * @brief Set the instruction type
 *
 * Set the type of the instruction given as input.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction to consider
 * @param type Type to set to the IR instruction given as input
 */
void IRMETHOD_setInstructionType (ir_method_t *method, ir_instruction_t *inst, JITUINT32 type);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Get the type of a IR instruction
 *
 * Return the instruction type of the IR instruction \c inst (e.g. IRNOP).
 *
 * @param inst IR instruction to consider
 * @return Type of the instruction given as input
 */
JITUINT16 IRMETHOD_getInstructionType (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the number of parameters of a IR instruction
 *
 * Return the number of parameters stored inside the instruction given as input
 *
 * @param inst IR instruction to consider
 * @return Number of parameters used by \c inst
 */
JITUINT32 IRMETHOD_getInstructionParametersNumber (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Circuit_Induction_Variables
 * @brief Check if a variable is inductive
 *
 * Check if variable \c varID is an induction variable within the circuit \c circuit.
 *
 * @param circuit IR circuit to consider
 * @param varID IR variable to check
 * @param method IR method that includes the variable \c varID and the circuit given as input
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isAnInductionVariable (ir_method_t *method, IR_ITEM_VALUE varID, circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit_Induction_Variables
 * @brief Get the list of induction variables
 *
 * Return the list of induction variables of the circuit given as input.
 *
 * The user is in charge to destroy the returned memory when it is not necessary anymore in the following way:
 * @code
 * XanList *l;
 * l = IRMETHOD_getCircuitInductionVariables(method, circuit);
 * ...
 * xanList_destroyList(l);
 * @endcode
 *
 * @param circuit IR circuit included in \c method
 * @param method IR method that includes the circuit given as input
 * @return List of (induction_variable_t *)
 */
XanList * IRMETHOD_getCircuitInductionVariables (ir_method_t *method, circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit_Induction_Variables
 * @brief Delete induction variables analysis
 *
 * Free the memory used to store information about induction variables identified in any circuits of the IR method <code> method </code>.
 *
 * @param method IR method to consider
 */
void IRMETHOD_deleteCircuitsInductionVariablesInformation (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Circuit_Invariants
 * @brief Get the list of invariants of a circuit
 *
 * Return the list of instructions that are invariants within the circuit given as input
 *
 * @param circuit IR circuit included in \c method
 * @param method IR method that includes the circuit given as input
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getCircuitInvariants (ir_method_t *method, circuit_t *circuit);

/**
 * \ingroup IRMETHOD_Circuit_Invariants
 * @brief Check if an instruction is invariant
 *
 * Check if the instruction <code> inst </code> is invariant within the circuit <code> circuit </code>.
 *
 * @param circuit IR circuit included in <code> method </code> to consider
 * @param method IR method that includes the circuit given as input
 * @param inst Instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isCircuitInvariant (ir_method_t *method, circuit_t *circuit, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Circuit
 * @brief Get all instructions that belong to any circuit with the same header.
 *
 * Get all instructions that belong to any circuit with the same header.
 *
 * @param method IR method to consider
 * @param header Header of considered circuits
 * @return List of (ir_instruction_t *)
 */
XanList * IRMETHOD_getInstructionsWithinAnyCircuitsOfSameHeader (ir_method_t *method, ir_instruction_t *header);

/**
 * \ingroup IRMETHOD_Circuit_Invariants
 * @brief Delete invariants analysis
 *
 * Free the memory used to store information about circuit invariants identified in any circuits of the IR method <code> method </code>.
 *
 * @param method IR method to consider
 */
void IRMETHOD_deleteCircuitsInvariantsInformation (ir_method_t *method);

/**
 * \ingroup IRMETHOD_MethodBody
 * @brief Check if a IR method can have a body
 *
 * Check if the method given as input can have a body composed by IR instructions.
 * For example if the method is an abstract method, this function will return JITFALSE.
 *
 * @param method IR method to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_hasIRMethodBody (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_entryPoint
 * @brief Get the function pointer
 *
 * Return the function pointer of the IR method given as input.
 * This function can be used to call directly from C the method.
 *
 * The returned pointer can be considered as the closure of the method given as input.
 *
 * This function can be used as following:
 * @code
 * void *fp;
 * int ret;
 * int inputValue;
 * inputValue = 5;
 *
 * fp = IRMETHOD_getFunctionPointer(method);
 *
 * ret = (*fp)(inputValue);
 * @endcode
 *
 * @param method IR Method to consider
 * @return Function pointer of \c compiledMethod
 */
void * IRMETHOD_getFunctionPointer (ir_method_t * method);

/**
 * \ingroup IRMETHOD_Method_entryPoint
 * @brief Set a method whether or not its entry point is referenced.
 *
 * Set a method whether or not its entry point is referenced.
 *
 * By default, all methods are not directly referenced.
 *
 * A method is directly referenced if its entry point address is directly used in instructions.
 *
 * Hence, by dumping IR instructions of the program, if an entry point of a method belongs to any parameter/result of any instruction, then that method is directly referenced.
 *
 * Notice that this is different than indirect calls where the entry point is indirectly used.
 *
 * @param method Method to set
 * @param isReferenced JITTRUE or JITFALSE
 */
void IRMETHOD_setMethodToHaveItsEntryPointDirectlyReferenced (ir_method_t * method, JITBOOLEAN isReferenced);

/**
 * \ingroup IRMETHOD_Method_entryPoint
 * @brief Check whether a method has its entry point directly referenced.
 *
 * Check whether a method has its entry point directly referenced.
 *
 * @param method Method to consider
 * @result JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_doesMethodHaveItsEntryPointDirectlyReferenced (ir_method_t * method);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the branch destination of a jump instruction
 *
 * This function set the branch destination of a <code> IRBRANCH </code> , <code> IRBRANCHIF</code>, <code> IRBRANCHIFNOT </code> and <code> IRBRANCHIFPCNOTINRANGE </code> instruction to the label given as input.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction to consider
 * @param labelID label to use
 */
void IRMETHOD_setBranchDestination (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE labelID);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the branch destination of a jump instruction
 *
 * This function get the branch destination of a <code> IRBRANCH </code> , <code> IRBRANCHIF</code>, <code> IRBRANCHIFNOT </code> and <code> IRBRANCHIFPCNOTINRANGE </code> instruction.
 * The destination will be always a label (i.e.,  IRLABEL).
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction to consider
 * @return Label where the instruction \c inst may jump to
 */
ir_instruction_t * IRMETHOD_getBranchDestination (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionCFG
 * @brief Get the fall through successor for a jump instruction
 *
 * This function get the fall through instruction for a <code> IRBRANCHIF</code>, <code> IRBRANCHIFNOT </code> and <code> IRBRANCHIFPCNOTINRANGE </code> instruction.
 *
 * @param method IR method that includes the instruction \c inst
 * @param inst IR instruction to consider
 * @return Successor instruction that \c inst may fall through to
 */
ir_instruction_t * IRMETHOD_getBranchFallThrough (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the Input Language type of a parameter of the method
 *
 * Return the descriptor of the Input Language (IL) data type of a parameter of the IR method.
 *
 * @param method IR method to consider
 * @param parameterNumber Number of the parameter to consider
 * @return Type descriptor of the <code> parameterNumber </code> parameter of the method
 */
TypeDescriptor * IRMETHOD_getParameterILType (ir_method_t * method, JITUINT32 parameterNumber);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the Input Language descriptor of a parameter of the method
 *
 * Return the descriptor of the Input Language (IL) method parameter.
 *
 * @param method IR method to consider
 * @param parameterNumber Number of the parameter to consider
 * @return Descriptor of the <code> parameterNumber </code> parameter of the method
 */
ParamDescriptor * IRMETHOD_getParameterILDescriptor (ir_method_t * method, JITUINT32 parameterNumber);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the Input Language type of a declared variable of a method
 *
 * Return the descriptor of the Input Language (IL) data type of a variable declared in IL of the IR method <code> method </code>.
 *
 * @param method IR method to consider
 * @param varID Variable ID
 * @return Descriptor of the variable <code> varID </code> of the method <code> method </code>
 */
TypeDescriptor * IRMETHOD_getDeclaredVariableILType (ir_method_t * method, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Method_Variables
 * @brief Get the IR type of a declared variable of a method
 *
 * Return the IR type of a variable declared in IL of the IR method <code> method </code>.
 *
 * @param method IR method to consider
 * @param varID Variable ID
 * @return IR type of the variable <code> varID </code>
 */
JITUINT16 IRMETHOD_getDeclaredVariableType (ir_method_t * method, IR_ITEM_VALUE varID);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Get the Input Language type of the result of the method
 *
 * Return the descriptor of the Input Language (IL) of the result of the method <code> method </code>.
 */
TypeDescriptor * IRMETHOD_getResultILType (ir_method_t * method);

/**
 * \ingroup IRMETHOD_Method_Signature
 * @brief Return the IR type of the result of the method
 *
 * Return the IR result type of the method.
 *
 * @param method IR method to consider
 * @return IR type of the result of the method
 */
JITUINT32 IRMETHOD_getResultType (ir_method_t *method);

/**
 * \ingroup IRMETHOD_Method_Signature
 *
 * Return the internal type of the call parameter number call_param
 */
JITUINT32 IRMETHOD_getParameterType (ir_method_t * method, JITUINT32 position);

/**
 * \ingroup IRMETHOD_ModifyMethodsSignature
 * @brief Allocate a signature
 *
 * Initialize an IR Method Signature.
 *
 * @param signature Signature to modify
 * @param parametersNumber Number of parameters of the signature to allocate
 */
void IRMETHOD_initSignature (ir_signature_t *signature, JITUINT32 parametersNumber);


/**
 * \ingroup IRMETHOD_ModifyMethodsSignature
 * @brief Set the parameter type of a method
 *
 * Set the type of the parameter of the method given as input.
 *
 * @param method IR method to consider
 * @param position Number of the parameter to set
 * @param value Type of the parameter to set
 */
void IRMETHOD_setParameterType (ir_method_t * method, JITUINT32 position, JITUINT32 value);

/**
 * \ingroup IRMETHOD_ModifyMethodsSignature
 * @brief Set the return type of a method
 *
 * Set the return type of the method <code> method </code>.
 *
 * @param method IR method to consider
 * @param value Type of the return to set
 */
void IRMETHOD_setResultType (ir_method_t *method, JITUINT32 value);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_cp
 * @brief Copy a IR variable/constant from an instruction to another one
 *
 * Copy one instruction's parameter into a (possibly different) parameter of another instruction.
 *
 * Parameter zero represents the return parameter.
 *
 * @param method IR Method that contain the instruction <code> toInst </code>
 * @param fromInst IR instruction where the IR variable/constant to copy comes from
 * @param fromParam Parameter number (or result) of instruction <code> fromInst </code> to copy
 * @param toInst IR instruction where the IR variable/constant is copied to
 * @param toParam Parameter number (or result) of instruction <code> toInst </code> where the IR variable <code> fromParam </code> is copied to
 */
void IRMETHOD_cpInstructionParameter (ir_method_t *method, ir_instruction_t *fromInst, JITUINT8 fromParam, ir_instruction_t *toInst, JITUINT8 toParam);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_cp
 * @brief Copy a IR variable/constant from an instruction to an IR item
 *
 * Copy one parameter of instruction @c fromInst to a (possibly different) IR item (i.e., @c toItem).
 *
 * Parameter zero represents the return parameter.
 *
 * @param method IR Method that contain the instruction <code> toInst </code>
 * @param fromInst IR instruction where the IR variable/constant to copy comes from
 * @param fromParam Parameter number (or result) of instruction <code> fromInst </code> to copy
 * @param toItem Destination IR item.
 */
void IRMETHOD_cpInstructionParameterToItem (ir_method_t *method, ir_instruction_t *fromInst, JITUINT8 fromParam, ir_item_t *toItem);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters
 * @brief Swap two parameters of an IR instruction
 *
 * Swap two parameters of an IR instruction.
 *
 * Parameter zero represents the return parameter.
 *
 * For example, consider the following code:
 * @code
 * IRMETHOD_swapInstructionParameters(method, inst, 1, 2);
 * @endcode
 *
 * The above code transforms the instruction
 * @code
 * IRMOVE(var1, var2)
 * @endcode
 * to
 * @code
 * IRMOVE(var2, var1)
 * @endcode
 *
 * @param method Method that contain the instruction <code> inst </code>
 * @param inst Instruction to consider
 * @param firstParam First parameter to consider
 * @param secondParam Second parameter to consider
 */
void IRMETHOD_swapInstructionParameters (ir_method_t *method, ir_instruction_t *inst, JITUINT8 firstParam, JITUINT8 secondParam);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_cp
 * @brief Copy a IR variable/constant to the first parameter of an instruction
 *
 * Copy a IR variable/constant to the first parameter of an instruction
 * @param inst IR instruction to consider to copy to
 * @param from IR variable or constant to copy from
 */
void IRMETHOD_cpInstructionParameter1 (ir_instruction_t *inst, ir_item_t * from);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_cp
 * @brief Copy a IR variable/constant to the second parameter of an instruction
 *
 * Copy a IR variable/constant to the second parameter of an instruction
 *
 * @param inst IR instruction to consider to copy to
 * @param from IR variable or constant to copy from
 */
void IRMETHOD_cpInstructionParameter2 (ir_instruction_t *inst, ir_item_t * from);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_cp
 * @brief Copy a IR variable/constant to the third parameter of an instruction
 *
 * Copy a IR variable/constant to the third parameter of an instruction
 *
 * @param inst IR instruction to consider to copy to
 * @param from IR variable or constant to copy from
 */
void IRMETHOD_cpInstructionParameter3 (ir_instruction_t *inst, ir_item_t * from);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_cp
 * @brief Copy a IR variable/constant to the result of an instruction
 *
 * Copy a IR variable/constant to the result of an instruction
 *
 * @param inst IR instruction to consider to copy to
 * @param from IR variable or constant to copy from
 */
void IRMETHOD_cpInstructionResult (ir_instruction_t *inst, ir_item_t * from);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set a parameter or result of an instruction
 *
 * Set the parameter <code> parameterNumber </code> of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 * Parameter zero represents the return parameter.
 *
 * The <code> typeInfos </code> can be <code> NULL </code> if no information is known.
 *
 * @param method IR Method that contain the instruction <code> fromInst </code>
 * @param inst IR instruction to consider
 * @param value Integer constant or variable ID to use to set the parameter <code> parameterNumber </code>
 * @param fvalue Floating point constant to use to set the parameter <code> parameterNumber </code>
 * @param type Type of the constant or IROFFSET if it is a variable to use to set the parameter <code> parameterNumber </code>
 * @param internal_type Type of the constant or variable to use to set the parameter <code> parameterNumber </code>
 * @param typeInfos Information about the type of the parameter to set
 * @param parameterNumber Number of the parameter to set
 */
void IRMETHOD_setInstructionParameter (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos, JITUINT16 parameterNumber);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set a parameter or result of an instruction with a new variable
 *
 * Set the parameter <code> parameterNumber </code> of the instruction <code> inst </code>, which belongs to the IR method <code> method </code> with a new variable of type specified as input.
 *
 * Parameter zero represents the return parameter.
 *
 * The <code> typeInfos </code> can be <code> NULL </code> if no information is known.
 *
 * @param method IR Method that contain the instruction <code> fromInst </code>
 * @param inst IR instruction to consider
 * @param internal_type Type of the constant or variable to use to set the parameter <code> parameterNumber </code>
 * @param typeInfos Information about the type of the parameter to set
 * @param parameterNumber Number of the parameter to set
 */
void IRMETHOD_setInstructionParameterWithANewVariable (ir_method_t *method, ir_instruction_t * inst, JITUINT16 internal_type, TypeDescriptor *typeInfos, JITUINT16 parameterNumber);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters
 * @brief Reset the fields of a parameter of an instruction
 *
 * Erase to the initial values each fields of the parameter <code> parameter </code> of the instruction <code> inst </code>.
 *
 * Parameter zero represents the return parameter.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst IR instruction to consider
 * @param parameterNumber Number of the parameter to reset (Parameter zero represents the return parameter).
 */
void IRMETHOD_eraseInstructionParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT16 parameterNumber);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the first parameter of an instruction
 *
 * Set the first parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * The <code> typeInfos </code> can be <code> NULL </code> if no information is known.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the first parameter or the ID of the variable stored as first parameter
 * @param fvalue Floating point value of the first parameter considered as a constant
 * @param type Type of the first parameter or <code> IROFFSET </code> if it is a variable
 * @param internal_type Type of the first parameter
 * @param typeInfos Information about the type of the first parameter
 */
void IRMETHOD_setInstructionParameter1 (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the value of the first parameter of an instruction
 *
 * Set the value of the first parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the first parameter or the ID of the variable stored as first parameter
 */
void IRMETHOD_setInstructionParameter1Value (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the ID of the variable defined by an instruction
 *
 * Set the ID of the variable defined by the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value ID of the variable defined by the instruction @c inst
 */
void IRMETHOD_setInstructionResultValue (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE value);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the type of the result of an instruction
 *
 * Set the type of the result of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param type Type of the result
 */
void IRMETHOD_setInstructionResultType (ir_method_t *method, ir_instruction_t * inst, JITINT16 type);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the type of the first parameter of an instruction
 *
 * Set the type of the first parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param type Type of the first parameter
 */
void IRMETHOD_setInstructionParameter1Type (ir_method_t *method, ir_instruction_t * inst, JITINT16 type);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the type of the second parameter of an instruction
 *
 * Set the type of the second parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param type Type of the second parameter
 */
void IRMETHOD_setInstructionParameter2Type (ir_method_t *method, ir_instruction_t * inst, JITINT16 type);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the type of the third parameter of an instruction
 *
 * Set the type of the third parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param type Type of the third parameter
 */
void IRMETHOD_setInstructionParameter3Type (ir_method_t *method, ir_instruction_t * inst, JITINT16 type);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the value of the second parameter of an instruction
 *
 * Set the value of the second parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the second parameter or the ID of the variable stored as second parameter
 */
void IRMETHOD_setInstructionParameter2Value (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the value of the third parameter of an instruction
 *
 * Set the value of the third parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the third parameter or the ID of the variable stored as third parameter
 */
void IRMETHOD_setInstructionParameter3Value (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the value of the result of an instruction
 *
 * Set the value of the result of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the result or the ID of the variable stored as result
 */
void IRMETHOD_setInstructionParameterResultValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the float value of the first parameter of an instruction
 *
 * Set the float value of the first parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param fvalue Floating point value of the first parameter or the ID of the variable stored as first parameter
 */
void IRMETHOD_setInstructionParameter1FValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_FVALUE fvalue);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the float value of the second parameter of an instruction
 *
 * Set the float value of the second parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param fvalue Floating point value of the second parameter or the ID of the variable stored as second parameter
 */
void IRMETHOD_setInstructionParameter2FValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_FVALUE fvalue);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the float value of the third parameter of an instruction
 *
 * Set the float value of the third parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param fvalue Floating point value of the third parameter or the ID of the variable stored as third parameter
 */
void IRMETHOD_setInstructionParameter3FValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_FVALUE fvalue);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the float value of the result of an instruction
 *
 * Set the float value of the result of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param fvalue Floating point value of the result or the ID of the variable stored as result
 */
void IRMETHOD_setInstructionParameterResultFValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE fvalue);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the second parameter of an instruction
 *
 * Set the second parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * The <code> typeInfos </code> can be <code> NULL </code> if no information is known.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the second parameter or the ID of the variable stored as second parameter
 * @param fvalue Floating point value of the second parameter considered as a constant
 * @param type Type of the second parameter or <code> IROFFSET </code> if it is a variable
 * @param internal_type Type of the second parameter
 * @param typeInfos Information about the type of the second parameter
 */
void IRMETHOD_setInstructionParameter2 (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the third parameter of an instruction
 *
 * Set the third parameter of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * The <code> typeInfos </code> can be <code> NULL </code> if no information is known.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the third parameter or the ID of the variable stored as third parameter
 * @param fvalue Floating point value of the third parameter considered as a constant
 * @param type Type of the third parameter or <code> IROFFSET </code> if it is a variable
 * @param internal_type Type of the third parameter
 * @param typeInfos Information about the type of the third parameter
 */
void IRMETHOD_setInstructionParameter3 (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos);

/**
 * \ingroup IRMETHOD_ModifyInstructionParameters_set
 * @brief Set the result of an instruction
 *
 * Set the result of the instruction <code> inst </code>, which belongs to the IR method <code> method </code>.
 *
 * The <code> typeInfos </code> can be <code> NULL </code> if no information is known.
 *
 * @param method IR method that includes the instruction <code> inst </code>
 * @param inst Instruction to modify
 * @param value Integer value of the result or the ID of the variable stored as result
 * @param fvalue Floating point value of the result considered as a constant
 * @param type Type of the result or <code> IROFFSET </code> if it is a variable
 * @param internal_type Type of the result
 * @param typeInfos Information about the type of the result
 */
void IRMETHOD_setInstructionResult (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get a parameter of an instruction
 *
 * Return the parameter <code> parNum </code> of the instruction <code> inst </code>.
 *
 * Parameter zero represents the return parameter.
 *
 * Parameters are numbered starting from 1.
 * Hence, the following call returns the first parameter of the instruction:
 * @code
 * IRMETHOD_getInstructionParameter(inst, 1);
 * @endcode
 *
 * @param inst Instruction to consider
 * @param parNum Number of the parameter to consider
 * @return The parameter <code> parNum </code> of the instruction given as input
 */
ir_item_t * IRMETHOD_getInstructionParameter (ir_instruction_t * inst, JITUINT32 parNum);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the first parameter of an instruction
 *
 * Return the first parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return First parameter of \c inst
 */
ir_item_t * IRMETHOD_getInstructionParameter1 (ir_instruction_t * inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the second parameter of an instruction
 *
 * Return the second parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Second parameter of \c inst
 */
ir_item_t * IRMETHOD_getInstructionParameter2 (ir_instruction_t * inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the third parameter of an instruction
 *
 * Return the third parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Third parameter of \c inst
 */
ir_item_t * IRMETHOD_getInstructionParameter3 (ir_instruction_t * inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the value of the first parameter of an instruction
 *
 * Return the value of the first parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Value of the first parameter of \c inst
 */
IR_ITEM_VALUE IRMETHOD_getInstructionParameter1Value (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the value of the second parameter of an instruction
 *
 * Return the value of the second parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Value of the second parameter of \c inst
 */
IR_ITEM_VALUE IRMETHOD_getInstructionParameter2Value (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the value of the third parameter of an instruction
 *
 * Return the value of the third parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Value of the third parameter of \c inst
 */
IR_ITEM_VALUE IRMETHOD_getInstructionParameter3Value (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the floating value of the first parameter of an instruction
 *
 * Return the floating value of the first parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Floating point value of the first parameter of \c inst
 */
IR_ITEM_FVALUE IRMETHOD_getInstructionParameter1FValue (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the floating value of the second parameter of an instruction
 *
 * Return the floating value of the second parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Floating point value of the second parameter of \c inst
 */
IR_ITEM_FVALUE IRMETHOD_getInstructionParameter2FValue (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the floating value of the third parameter of an instruction
 *
 * Return the floating value of the third parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Floating point value of the third parameter of \c inst
 */
IR_ITEM_FVALUE IRMETHOD_getInstructionParameter3FValue (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the type of the first parameter of an instruction
 *
 * Return the type of the first parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Type of the first parameter of \c inst
 */
JITUINT32 IRMETHOD_getInstructionParameter1Type (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the type of the second parameter of an instruction
 *
 * Return the type of the second parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Type of the second parameter of \c inst
 */
JITUINT32 IRMETHOD_getInstructionParameter2Type (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_InstructionParameters
 * @brief Get the type of the third parameter of an instruction
 *
 * Return the type of the third parameter of the instruction.
 *
 * @param inst Instruction to consider
 * @return Type of the third parameter of \c inst
 */
JITUINT32 IRMETHOD_getInstructionParameter3Type (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Program
 * @brief Get the name of the input program
 *
 * This function returns the name of the running program.
 *
 * @return Name of the program that is running
 */
JITINT8 * IRPROGRAM_getProgramName (void);

/**
 * \ingroup IRMETHOD_Program
 * @brief Free the memory used for CFG circuits
 *
 * Free the memory used for CFG circuits.
 */
void IRPROGRAM_deleteCircuitsInformation (void);

/**
 * \ingroup IRMETHOD_IRCodeCache
 * @brief Save the program to the filesystem
 *
 * This function save the IR code of the whole program within the directory specified by <code> directoryName </code>.
 *
 * If <code> directoryName </code> is equal to <code> NULL </code>, then the default directory is used.
 *
 * Example:
 * @code
 * IRPROGRAM_saveProgram("/home/me/myIRcode");
 * @endcode
 *
 * @param directoryName Name of the directory where to save the program
 */
void IRPROGRAM_saveProgram (JITINT8 *directoryName);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Fetch the method that initialize the static memories of a given IL type
 *
 * Fetch the method that initialize the static memories of a given IL type.
 *
 * @param ilType IL Type to consider
 * @return Method used to initialize the static memory of @c ilType
 */
ir_method_t * IRDATA_getInitializationMethodOfILType (TypeDescriptor *ilType);

/**
 * \ingroup IRMETHOD_DataType
 * @brief Fetch the method that initialize the static memories of a given IL type
 *
 * Fetch the method that initialize the static memories of a given IL type.
 *
 * @param item IR item with the IL type to consider
 * @return Method used to initialize the static memory of the IL type instance stored in @c item
 */
ir_method_t * IRDATA_getInitializationMethodOfILTypeFromIRItem (ir_item_t *item);

/**
 * \ingroup IRMETHOD_DataTypeTypeHierarchy
 * @brief Check if a variable of a given type can be assigned to another variable with another given type
 *
 * This function returns JITTRUE if a variable of type <code> typeID </code> can be assigned to another variable of type <code> desiredTypeID </code>.
 *
 * Return JITFALSE otherwise.
 *
 * @param typeID Type of a source variable
 * @param desiredTypeID Type of a destination variable
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isAssignable (TypeDescriptor * typeID, TypeDescriptor *desiredTypeID);

/**
 * \ingroup IRMETHOD_DataTypeTypeHierarchy
 * @brief Check if a given type is a subtype of another one
 *
 * This function returns JITTRUE if the type <code> subTypeID </code> is a subtype of the type <code> superTypeID </code>.
 *
 * Return JITFALSE otherwise.
 *
 * @param superTypeID A input language type
 * @param subTypeID A input language type
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRDATA_isSubtype (TypeDescriptor *superTypeID, TypeDescriptor * subTypeID);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Allocate data sets.
 *
 * Allocate sets required for data flow analysis.
 *
 * The caller is in charge to destroy the returned sets by calling \ref DATAFLOW_destroySets .
 *
 * For example:
 * @code
 * data_flow_t	*df;
 * JITUINT32 instructionsNumber;
 *
 * // Allocate the sets attached to each instruction where each set has 10 elements
 * instructionsNumber = IRMETHOD_getInstructionsNumber(m);
 * df = DATAFLOW_allocateSets(instructionsNumber, 10);
 *
 * // Use the sets
 * ...
 *
 * // Destroy the sets
 * DATAFLOW_destroySets(df);
 *
 * @endcode
 *
 * @param instructionsNumber Number of instructions of the method that is going to be analyzed with the returned data sets.
 * @param elements Maximum number of elements of each set
 * @return Data sets
 */
data_flow_t * DATAFLOW_allocateSets (JITUINT32 instructionsNumber, JITUINT32 elements);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Add an element to a GEN set.
 *
 * Add @c elementID to the set @c setID stored in the data flow @c sets.
 *
 * For example, if we are using data flow with sets per instruction and these sets include variables (e.g., to compute variable defined), then:
 * @code
 *
 * // Fetch an instruction
 * ir_instruction_t *i = IRMETHOD_getInstructionAtPosition(method, 5);
 *
 * // Check if the instruction defines a variable
 * if (IRMETHOD_doesInstructionDefineAVariable(method, i)){
 *
 * 	//Fetch the variable defined by the instruction
 *	varID   = IRMETHOD_getVariableDefinedByInstruction(method, i);
 *
 * 	// Add the variable varID to the GEN set of the instruction i
 * 	DATAFLOW_addElementToGENSet(sets, IRMETHOD_getInstructionPosition(i), varID);
 * }
 *
 * @endcode
 *
 * @param sets Data flow
 * @param setID Specific set of @c sets to consider.
 * @param elementID Element to add to the @c setID set of @c sets.
 */
void DATAFLOW_addElementToGENSet (data_flow_t *sets, JITUINT32 setID, JITUINT32 elementID);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Check whether an element exists in a IN set.
 *
 * Check whether @c elementID exists in the set @c setID stored in the data flow @c sets.
 *
 * For example, if we are using data flow with sets per instruction and these sets include variables (e.g., to compute variable defined), then:
 * @code
 *
 * // Check whether variable 7 exists in IN set of instruction 5
 * DATAFLOW_addElementToGENSet(sets, 5, 7);
 *
 * @endcode
 *
 * @param sets Data flow
 * @param setID Specific set of @c sets to consider.
 * @param elementID Element to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN DATAFLOW_doesElementExistInINSet (data_flow_t *sets, JITUINT32 setID, JITUINT32 elementID);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set some sets to be full
 *
 * Set specified sets (i.e., GEN, KILL, IN, OUT) to be full.
 *
 * For example, to set IN sets of all elements to be full:
 * @code
 * DATAFLOW_setAllFullSets(sets, JITFALSE, JITFALSE, JITTRUE, JITFALSE);
 * @endcode
 *
 * @param sets Sets to consider
 * @param gen JITTRUE or JITFALSE
 * @param kill JITTRUE or JITFALSE
 * @param in JITTRUE or JITFALSE
 * @param out JITTRUE or JITFALSE
 */
void DATAFLOW_setAllFullSets (data_flow_t *sets, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set some sets to be full
 *
 * Set specified sets (i.e., GEN, KILL, IN, OUT) of a specific element to be full.
 *
 * For example, to set IN sets of element 5 to be full:
 * @code
 * DATAFLOW_setFullSets(sets, 5, JITFALSE, JITFALSE, JITTRUE, JITFALSE);
 * @endcode
 *
 * @param sets Sets to consider
 * @param setID Identifier of the set
 * @param gen JITTRUE or JITFALSE
 * @param kill JITTRUE or JITFALSE
 * @param in JITTRUE or JITFALSE
 * @param out JITTRUE or JITFALSE
 */
void DATAFLOW_setFullSets (data_flow_t *sets, JITUINT32 setID, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set some sets to be empty
 *
 * Set specified sets (i.e., GEN, KILL, IN, OUT) to be empty.
 *
 * For example, to set IN sets of all elements to be empty:
 * @code
 * DATAFLOW_setAllEmptySets(sets, JITFALSE, JITFALSE, JITTRUE, JITFALSE);
 * @endcode
 *
 * @param sets Sets to consider
 * @param gen JITTRUE or JITFALSE
 * @param kill JITTRUE or JITFALSE
 * @param in JITTRUE or JITFALSE
 * @param out JITTRUE or JITFALSE
 */
void DATAFLOW_setAllEmptySets (data_flow_t *sets, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Set some sets to be empty
 *
 * Set specified sets (i.e., GEN, KILL, IN, OUT) of a specific element to be empty.
 *
 * For example, to set IN sets of element 5 to be empty:
 * @code
 * DATAFLOW_setEmptySets(sets, 5, JITFALSE, JITFALSE, JITTRUE, JITFALSE);
 * @endcode
 *
 * @param sets Sets to consider
 * @param setID Identifier of the set
 * @param gen JITTRUE or JITFALSE
 * @param kill JITTRUE or JITFALSE
 * @param in JITTRUE or JITFALSE
 * @param out JITTRUE or JITFALSE
 */
void DATAFLOW_setEmptySets (data_flow_t *sets, JITUINT32 setID, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Free memory used for any data flow analysis.
 *
 * Free memory used for any data flow analysis.
 *
 * @param method Method to consider
 */
void IRMETHOD_deleteDataFlowInformation (ir_method_t *method);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Free memory used for any data flow analysis for the entire program.
 *
 * Free memory used for any data flow analysis for the entire program.
 */
void IRPROGRAM_deleteDataFlowInformation (void);

/**
 * \ingroup IRMETHODMETADATA_DataFlow
 * @brief Destroy a data set.
 *
 * Destroy data set @c sets previously allocated by \ref DATAFLOW_allocateSets .
 *
 * @param sets Previously allocated set that will be destroy.
 */
void DATAFLOW_destroySets (data_flow_t *sets);

/**
 * \ingroup IRMETHOD_Execution
 * @brief Execute a IR method
 *
 * This function execute the already compiled method <code> compiledMethod </code> and store the return value to <code> returnArea </code>.
 *
 * This function can be used as following:
 * @code
 * int ret;
 * int input;
 * void *args[1];
 * input = 5;
 * args[0] = &input;
 *
 * IRMETHOD_executeMethod(method, args, &ret);
 * @endcode
 *
 * @param method Pointer to IR Method to be executed
 * @param args Arguments to use
 * @param returnArea pointer to the memory where the return value will be stored
 */
void IRMETHOD_executeMethod (ir_method_t *method, void **args, void *returnArea);

/**
 * \ingroup IRMETHOD_Execution
 * @brief Get the running method
 *
 * This function returns the IR Method that is currently in execution.
 *
 * @return IR method currently in execution
 */
ir_method_t * IRMETHOD_getRunningMethod ();

/**
 * \ingroup IRMETHOD_Execution
 * @brief Get the caller of a method in execution
 *
 * This function returns the caller of the method <code> m </code> that is currently in execution.
 *
 * @param m Method to consider
 * @return Caller of the method <code> m </code> that is currently in execution
 */
ir_method_t * IRMETHOD_getCallerOfMethodInExecution (ir_method_t *m);

/**
 * \ingroup IRMETHOD_Dump
 * @brief Return IR Instruction or IR Type of the name given as input
 *
 * Convert keyword from name to internal rappresentation.
 *
 * @param name the name to consider
 * @return identifier of keywork
 */
JITINT32 IRMETHOD_getValueFromIRKeyword (JITINT8 *name);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction performs a volatile operation
 *
 * Return JITTRUE if the instruction has been set as volatile by calling the function \c IRMETHOD_setInstructionAsVolatile .
 *
 * Return JITFALSE otherwise.
 *
 * @param inst Instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isInstructionVolatile (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Check if an instruction performs a call to a vararg method
 *
 * Return JITTRUE if the instruction has been set as a call to a vararg method by calling the function \c IRMETHOD_setInstructionAsVarArg .
 *
 * Return JITFALSE otherwise.
 *
 * @param inst Instruction to consider
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IRMETHOD_isACallInstructionToAVarArgMethod (ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ModifyInstructions
 * @brief Set or unset the instruction as volatile
 *
 * If \c isVolatile is JITTRUE, this function set the instruction \c inst as volatile.
 *
 * Otherwise, it sets \c inst as not volatile.
 *
 * Instructions are created as not volatile by default.
 *
 * @param method Method that includes the instruction given as input (i.e., \c inst)
 * @param inst Instruction to consider
 * @param isVolatile JITTRUE or JITFALSE
 */
void IRMETHOD_setInstructionAsVolatile (ir_method_t *method, ir_instruction_t *inst, JITBOOLEAN isVolatile);

/**
 * \ingroup IRMETHOD_ModifyInstructions
 * @brief Set or unset the instruction as a call to a vararg method
 *
 * If \c isVararg is JITTRUE, this function set the instruction \c inst as a call to a vararg method.
 *
 * Otherwise, it sets \c inst as not vararg.
 *
 * Instructions are created as not vararg by default.
 *
 * @param method Method that includes the instruction given as input (i.e., \c inst)
 * @param inst Instruction to consider
 * @param isVararg JITTRUE or JITFALSE
 */
void IRMETHOD_setInstructionAsVarArg  (ir_method_t *method, ir_instruction_t *inst, JITBOOLEAN isVararg);

/**
 * \ingroup IRMETHOD_Instruction
 * @brief Get the instruction distance
 *
 * Return the instruction distance of \c inst .
 *
 * The distance of an instruction is used by the DLA compiler.
 *
 * @param method Method that includes the instruction given as input (i.e., \c inst)
 * @param inst Instruction to consider
 * @return Distance of the instruction
 */
JITINT16 IRMETHOD_getInstructionDistance (ir_method_t *method, ir_instruction_t *inst);

/**
 * \ingroup IRMETHOD_ModifyInstructions
 * @brief Set the instruction distance
 *
 * Set the instruction distance to be \c distance .
 *
 * The distance of an instruction is used by the DLA compiler.
 *
 * @param method Method that includes the instruction given as input (i.e., \c inst)
 * @param inst Instruction to consider
 * @param distance Distance of the instruction
 */
void IRMETHOD_setInstructionDistance (ir_method_t *method, ir_instruction_t *inst, JITINT16 distance);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction to a C method before a set of instructions
 *
 * Allocate a new IRNATIVECALL instruction, which will call the C method specified in input with the return and parameters specified, before every instruction in the list <code> instructionsToConsider </code>.
 *
 * Consider the case that we want to call a C function called <code> my_C_function </code>.
 *
 * The code we need to add to the codetool is the following:
 * @code
 * void my_C_function (JITINT32 runtimeValue){
 *   printf("This code will be executed at run time %d\n", runtimeValue);
 *   return ;
 * }
 *
 * void dummy_do_job (ir_method_t * method){
 *   XanList *l;
 *   XanList *params;
 *   ir_item_t item;
 *
 *   l = IRMETHOD_getInstructions(method);
 *   params = xanList_new(allocFunction, freeFunction, NULL);
 *
 *   memset(&item, 0, sizeof(ir_item_t));
 *   item.value.v = 10;
 *   item.type = IRINT32;
 *   item.internal_type = item.type;
 *   xanList_append(params, &item);
 *
 *   IRMETHOD_addNativeCallInstructionsBeforeEveryInstruction(method, l, "my_C_function", my_C_function, NULL, params);
 *
 *   xanList_destroyList(params);
 *   xanList_destroyList(l);
 * }
 * @endcode
 *
 * @param method IR method where the new instruction will be inserted
 * @param instructionsToConsider Set of instructions to consider
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C methdo to call
 * @param returnItem Return of the method to call. If it is NULL, the return type IRVOID is considered. Otherwise, its content will be copied.
 * @param callParameters List of (ir_item_t *). These are call parameters to use in the call. The function \ref IRMETHOD_newInstructionBefore copies only their contents without linking to them or freeing them.
 */
void IRMETHOD_addNativeCallInstructionsBeforeEveryInstruction (ir_method_t *method, XanList *instructionsToConsider, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);

/**
 * \ingroup IRMETHOD_ModifyMethodsAddingCallInstructions
 * @brief Add a new call instruction to a C method before a set of instructions
 *
 * Allocate a new IRNATIVECALL instruction before every instruction in the list <code> instructionsToConsider </code>.
 *
 * The C method called must have the return specified as input by the parameter <code> returnItem </code> and the method and the instruction as its parameters.
 *
 * Consider the case that we want to call a C function called <code> my_C_function </code>.
 *
 * The code we need to add to the codetool is the following:
 * @code
 * void my_C_function (ir_method_t *method, ir_instruction_t *instructionExecuted){
 *   printf("This code will be executed at run time\n");
 *   return ;
 * }
 *
 * void dummy_do_job (ir_method_t * method){
 *   XanList *l;
 *   l = IRMETHOD_getInstructions(method);
 *   IRMETHOD_addNativeCallInstructionsBeforeEveryInstructionWithThemAsParameter(method, l, "my_C_function", my_C_function, NULL);
 *   xanList_destroyList(l);
 * }
 * @endcode
 *
 * Following there are some other examples of the C function that will call by the native call instructions injected:
 * @code
 * void my_C_function (ir_method_t *method, ir_instruction_t *instructionExecuted);
 * JITUINT32 my_C_function (ir_method_t *method, ir_instruction_t *instructionExecuted);
 * JITINT64 my_C_function (ir_method_t *method, ir_instruction_t *instructionExecuted);
 * JITFLOAT32 my_C_function (ir_method_t *method, ir_instruction_t *instructionExecuted);
 * void * my_C_function (ir_method_t *method, ir_instruction_t *instructionExecuted);
 * ...
 * @endcode
 *
 * @param method IR method where the new instruction will be inserted
 * @param instructionsToConsider Set of instructions to consider
 * @param functionToCallName Name of the C method to call. This is used for debug purpose usually (it can be NULL)
 * @param functionToCallPointer Pointer to the C methdo to call
 * @param returnItem Return of the method to call. If it is NULL, the return type IRVOID is considered. Otherwise, its content will be copied.
 */
void IRMETHOD_addNativeCallInstructionsBeforeEveryInstructionWithThemAsParameter (ir_method_t *method, XanList *instructionsToConsider, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem);

/**
 * \ingroup IRMETHOD_Loop
 * @brief Analyze loops of the entire program
 *
 * Analyze the loops of the entire program
 *
 * @return Hash table where (loop_t *) is both the key and the value
 */
XanHashTable * IRLOOP_analyzeLoops (void (*analyzeCircuits)(ir_method_t *m));

/**
 * \ingroup IRMETHOD_Loop
 * @brief Analyze loops of a given method
 *
 * Analyze the loops of method <code> method </code>
 *
 * @return Hash table where (loop_t *) is both the key and the value
 */
XanHashTable * IRLOOP_analyzeMethodLoops (ir_method_t *method, void (*analyzeCircuits)(ir_method_t *m));

/**
 * \ingroup IRMETHOD_Loop
 * @brief Get a loop that includes an instruction.
 *
 * @param loops Returned from \ref IRLOOP_analyzeLoops .
 * @return Loop that includes @c inst or NULL.
 */
loop_t * IRLOOP_getLoopFromInstruction (ir_method_t *method, ir_instruction_t *inst, XanHashTable *loops);

/**
 * \ingroup IRMETHOD_Loop
 * @brief Get outermost loops of a method.
 */
XanList * IRLOOP_getOutermostLoopsWithinMethod (ir_method_t *method, XanHashTable *loops);

/**
 * \ingroup IRMETHOD_Loop
 * @brief Get a loop that includes a circuit.
 */
loop_t * IRLOOP_getLoop (ir_method_t *method, circuit_t *loop, XanHashTable *loops);

/**
 * \ingroup IRMETHOD_Loop
 * @brief Compute information about a loop
 *
 * Compute information about a loop starting from information about circuits of the method
 * @code
 * loop->method
 * @endcode
 *
 * Usually this function is called to refresh information about a specific loop.
 *
 * This function requires \ref LOOP_IDENTIFICATION , \ref LOOP_INVARIANTS_IDENTIFICATION , and \ref INDUCTION_VARIABLES_IDENTIFICATION .
 *
 */
void IRLOOP_computeLoopInformation (loop_t *loop, JITBOOLEAN updateVariablesInformation, XanList *escapedMethods, XanList *reachableMethods);

/**
 * \ingroup IRMETHOD_Loop
 * @brief Refresh information about loops of the entire program
 *
 * Refresh information about loops of the entire program
 */
void IRLOOP_refreshLoopsInformation (XanHashTable *loops);

/**
 * \ingroup IRMETHOD_Loop
 * @brief Destroy information about all loops of the program
 *
 * Destroy information about all loops of the program.
 *
 * The parameter @c loops must have been returned by \ref IRLOOP_analyzeLoops .
 *
 * Hence, the use case if the following:
 * @code
 * XanHashTable *loops;
 *
 * // Identify all loops of the program
 * loops = IRLOOP_analyzeLoops(myFunctionToAnalyzeCircuits);
 *
 * // Use the information about loops
 * ..
 *
 * // Free the memory
 * IRLOOP_destroyLoops(loops);
 * @endcode
 *
 * @param loops All loops of a program
 */
void IRLOOP_destroyLoops (XanHashTable *loops);

/**
 * \ingroup IRMETHOD_Loop
 * @brief Destroy information about a loop
 */
void IRLOOP_destroyLoop (loop_t *loop);

#ifdef __cplusplus
};
#endif

#endif
