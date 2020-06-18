/*
 * Copyright (C) 2006 - 2010 Simone Campanoni
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
   *@file ir_language.h
 */
#ifndef IR_LANGUAGE_H
#define IR_LANGUAGE_H

/**
 * @defgroup IRLanguageInstructions Instruction set
 * \ingroup IRLanguage
 *
 * The IR language includes a set of IR instructions defined in this module. \n
 * Every instruction can have from 0 up to 3 parameters (i.e., param_1, param_2 and param_3).\n
 * Moreover, it can have unlimited number of ''call parameters'' (mainly used by call instructions).
 *
 * Every IR instruction type is described providing information about
 * <ul>
 * <li> the semantic of the instruction
 * <li> the parameters
 * <li> the call parameters
 * </ul>
 *
 */

/**
 * @defgroup IRGrammar Grammar
 * \ingroup IRLanguageInstructions
 *
 * Parameters and call parameters can be constants or variables; both are described by the following grammar.
 * Consider that Value, Type and InternalType used below are the fields of the structure <code> \ref ir_item_t </code> described in \ref IRLanguageDataTypes
 *
 * constType			=       IRINT8          |
 *                                      IRINT16         |
 *                                      IRINT32         |
 *                                      IRINT64         |
 *                                      IRNINT		|
 *                                      IRUINT8         |
 *                                      IRUINT16        |
 *                                      IRUINT32        |
 *                                      IRUINT64        |
 *                                      IRNUINT		|
 *                                      IRFLOAT32       |
 *                                      IRFLOAT64       |
 *                                      IRNFLOAT
 * \n
 * strictDecimalSingleNumber	=	1		|
 *                                      2		|
 *                                      3		|
 *                                      4		|
 *                                      5		|
 *                                      6		|
 *                                      7		|
 *                                      8		|
 *                                      9
 * \n
 * booleanNumber		=	0		|
 *                                      1
 * \n
 * decimalSingleNumber		=       0 strictDecimalSingleNumber
 * \n
 * decimalNumber		=       strictDecimalNumber decimalSingleNumber*
 * \n
 * floatingNumber		=       decimalNumber , decimalSingleNumber+
 * \n
 * varID			=       decimalNumber
 * \n
 * number			=       decimalNumber | floatingNumber
 * \n
 * epsilon			=
 *
 * IRVariable
 * <ul>
 * <li> Value		= varID
 * <li>	Type		= IROFFSET
 * <li> InternalType	= constType
 * </ul>
 *
 * IRConstant
 * <ul>
 * <li> Value		= number
 * <li> Type		= constType
 * <li> InternalType	= constType
 * </ul>
 *
 * IRMethod
 * <ul>
 * <li> Value		= decimalNumber
 * <li> Type		= IRMETHODID
 * <li> InternalType	= IRPOINTER
 * </ul>
 *
 * IRClass
 * <ul>
 * <li> Value		= decimalNumber
 * <li> Type		= IRCLASSID
 * <li> InternalType	= IRPOINTER
 * </ul>
 *
 * IRType
 * <ul>
 * <li> Value		= constType
 * <li> Type		= IRTYPE
 * <li> InternalType	= IRTYPE
 * </ul>
 *
 * IRFlag
 * <ul>
 * <li> Value		= booleanNumber
 * <li> Type		= IRBOOLEAN
 * <li> InternalType	= IRBOOLEAN
 * </ul>
 *
 * IRFlags
 * <ul>
 * <li> Value		= decimalNumber
 * <li> Type		= IRINT8
 * <li> InternalType	= IRINT8
 * </ul>
 *
 * IRLabel
 * <ul>
 * <li> Value		= decimalNumber
 * <li> Type		= IRLABELITEM
 * <li> InternalType	= IRLABELITEM
 * </ul>
 *
 * <hr>
 * <b> IR instructions </b>
 * <hr>
 *
 * Single IR instructions are defined within the following subsections based on their types.
 *
 * Next: \ref IRLanguageDataTypes
 */

/**
 * @defgroup IRInstructions Instructions
 * \ingroup IRLanguageInstructions
 */

/**
 * @defgroup IRLanguageMathInstructions Mathematic instruction set
 * \ingroup IRInstructions
 *
 * Instructions that perform any sort of mathematic operation belong to this set
 *
 */

/**
 * @defgroup IRLanguageInstructionsArith Arithmetic instruction set
 * \ingroup IRLanguageMathInstructions
 *
 * The instructions that belong to this set perform arithmetic operations on both integer and floating point variables or constants.
 *
 */

/**
 * @defgroup IRLanguageInstructionsCompare Compare instruction set
 * \ingroup IRInstructions
 *
 * The instructions that belong to this set perform comparison operations on both integer and floating point variables or constants.
 *
 */

/**
 * @defgroup IRLanguageInstructionsBitwise Bitwise instruction set
 * \ingroup IRLanguageMathInstructions
 *
 * The instructions that belong to this set perform bitwise operations on both variables and constants.
 *
 */

/**
 * @defgroup IRLanguageInstructionsJump Jump instruction set
 * \ingroup IRInstructions
 *
 * The instructions that belong to this set perform jumps to labels both within a single Control-Flow-Graph (CFG) or across CFGs (i.e., call instructions) that are not due to exceptions.
 *
 */

/**
 * @defgroup IRLanguageInstructionsCall Call instruction set
 * \ingroup IRLanguageInstructionsJump
 *
 * The instructions that belong to this set perform calls to methods.
 *
 */

/**
 * @defgroup IRLanguageInstructionsFinally Finally instruction set
 * \ingroup IRLanguageInstructionsJump
 *
 * The instructions that belong to this set perform calls to finally blocks.
 *
 */

/**
 * @defgroup IRLanguageInstructionsException Exception Instructions Set
 * \ingroup IRInstructions
 *
 * The instructions that belong to this set perform operations due to handle runtime exceptions (i.e., division by zero).
 *
 */

/**
 * @defgroup IRLanguageInstructionsExceptionFilter Filter instruction set
 * \ingroup IRLanguageInstructionsException
 *
 * The instructions that belong to this set perform operations related to filters, which are a special type of exception catcher.
 */

/**
 * @defgroup IRLanguageInstructionsExceptionThrow Throw exception instruction set
 * \ingroup IRLanguageInstructionsException
 *
 * The instructions that belong to this set perform operations related to exception throwing.
 */

/**
 * @defgroup IRLanguageInstructionsExceptionCatcher Exception catching instruction set
 * \ingroup IRLanguageInstructionsException
 *
 * The instructions that belong to this set perform operations needed to catch exceptions thrown at runtime.
 */

/**
 * @defgroup IRLanguageInstructionsMemoryAllocation Memory allocation instruction set
 * \ingroup IRLanguageInstructionsMemory
 *
 * The instructions that belong to this set allocate or free memory from both the heap and the call stack.
 *
 */

/**
 * @defgroup IRLanguageInstructionsMemoryAllocationManaged IR managed memory allocation instructions set
 * \ingroup IRLanguageInstructionsMemoryAllocation
 *
 * The instructions that belong to this set allocate or free memory managed by a garbage collection.
 *
 */

/**
 * @defgroup IRLanguageInstructionsMemory Memory instruction set
 * \ingroup IRInstructions
 *
 * The instructions that belong to this set perform operations on memory (either heap or stack).
 *
 */

/**
 * @defgroup IRLanguageInstructionsMemoryOperations Memory operation instruction set
 * \ingroup IRLanguageInstructionsMemory
 *
 * The instructions that belong to this set perform operations on values stored to memory (from both the heap and the call stack) previously allocated.
 *
 */

/**
 * @defgroup IRLanguageInstructionsArrayMemoryOperations Array instruction set
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * The instructions that belong to this set perform operations on arrays previously allocated.
 *
 */

/**
 * @defgroup IRLanguageInstructionsConv Type conversion instruction set
 * \ingroup IRInstructions
 *
 * Instructions that perform type conversions of runtime values.
 *
 */

/**
 * @defgroup IRLanguageInstructionsMisc Miscellaneous instruction set
 * \ingroup IRInstructions
 *
 * Instructions that does not fit in every other set belong to this one.
 *
 */

/**
 * @defgroup IRLanguageDataTypes Data types
 * \ingroup IRLanguage
 *
 * Data can be stored using the IR language in:
 * <ul>
 * <li> method variables
 * <li> constants
 * <li> symbols
 * <li> globals
 * </ul>
 * All data is represented by instances of \ref ir_item_t .
 * How to manage these data types is described in \ref IRMETHOD_DataType .
 *
 * In more details, IR variables, constants, symbols, and globals (i.e., \ref ir_item_t ) have all five fields: value, fvalue, type, internal_type, type_infos.
 * <ul>
 * <li> <code> type </code>: This field defines if this element is a variable, a constant, a symbol, or a global. If the IR item is a variable, type is equal to \ref IROFFSET. If it is a constant, then this field is equal to <code> internal_type </code>. Moreover, if it is a symbol, then it is equal to \ref IRSYMBOL. Finally, if it is a global element (either variable or constant), then it is equal to \ref IRGLOBAL.
 * <li> <code> internal_type </code>: this is the actual builtin type of the IR item (e.g., <code> \ref IRINT32 </code>)
 * <li> <code> value </code>: if the item is a constant, then this field stores the value of that constant. If the item is a variable (i.e., type is equal to \ref IROFFSET), then this field stores the identificator of the variable (i.e., its ID). If the item is a symbol, then this field points to the obfuscated data structure that internal describe the symbol (to use the symbol, use the API describes in \ref IRSYMBOLS).
 * <li> <code> fvalue </code>: This field is useful only for floating point constants. In this case, this field stores the floating point value of that constant.
 * <li> <code> type_infos </code>: this field reports the metadata description of the high level data type of the IR variable or constant (i.e., class description)
 * </ul>
 *
 * For example, the variable 5 which stores an integer 32 bits value will have
 * <ul>
 * <li> <code> type = \ref IROFFSET </code>
 * <li> <code> internal_type = \ref IRINT32 </code>
 * <li> <code> value = 5 </code>
 * <li> <code> type_infos = </code> high level metadata description of the integer 32 bits type
 * </ul>
 *
 * Moreover, the integer 32 bits constant equal to 425 will have
 * <ul>
 * <li> <code> type = \ref IRINT32 </code>
 * <li> <code> internal_type = \ref IRINT32 </code>
 * <li> <code> value = 425 </code>
 * <li> <code> type_infos = </code> high level metadata description of the integer 32 bits type
 * </ul>
 *
 * Finally, for the floating point 32 bits constant equal to 5.123, we will have
 * <ul>
 * <li> <code> type = \ref IRFLOAT32 </code>
 * <li> <code> internal_type = \ref IRFLOAT32 </code>
 * <li> <code> fvalue = 5.123 </code>
 * <li> <code> type_infos = </code> high level metadata description of the floating point 32 bits type
 * </ul>
 *
 * Next: \ref IRMETHOD
 */

/**
 * @defgroup IRLanguagePointerDataTypes Pointer data types
 * \ingroup IRLanguageDataTypes
 *
 * IR pointer data types are here described.
 */

/**
 * @defgroup IRLanguageFloatDataTypes Floating point data types
 * \ingroup IRLanguageDataTypes
 *
 * IR floating point data types are here described.
 */

/**
 * @defgroup IRLanguageIntDataTypes Integer data types
 * \ingroup IRLanguageDataTypes
 *
 * IR integer data types are here described.
 */

/**
 * @def IRMOVE
 * \ingroup IRLanguageInstructionsMisc
 *
 * Move the contents of the first parameter to the result of the instruction.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRMOVE         		1

/**
 * @def IRADD
 * \ingroup IRLanguageInstructionsArith
 *
 * Add two values together and return the result in a new temporary value. \n
 * The difference with \ref IRADDOVF is that no exception will be thrown in any case. \n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRADD           2

/**
 * @def IRADDOVF
 * \ingroup IRLanguageInstructionsArith
 *
 * Add two values together and return the result.\n
 * Throw an exception if overflow occurs. \n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRADDOVF        3

/**
 * @def IRSUB
 * \ingroup IRLanguageInstructionsArith
 *
 * Subtract two values and return the result.\n
 * The difference with \ref IRSUBOVF is that no exception will be thrown in any case. \n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRSUB           4

/**
 * @def IRSUBOVF
 * \ingroup IRLanguageInstructionsArith
 *
 * Subtract two values and return the result.\n
 * Throw an exception if overflow occurs.\n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRSUBOVF        5

/**
 * @def IRMUL
 * \ingroup IRLanguageInstructionsArith
 *
 * Multiply two values (i.e., \c param1 and \c param2) and store the result in \c result
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRMUL           6

/**
 * @def IRMULOVF
 * \ingroup IRLanguageInstructionsArith
 *
 * Multiply two values (i.e., \c param1 and \c param2) and store the result in \c result.
 *
 * Throw an exception if overflow occurs.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRMULOVF        7

/**
 * @def IRDIV
 * \ingroup IRLanguageInstructionsArith
 *
 * Divide two values (\c param1 and \c param2) and store the quotient in \c result.
 *
 * This instruction throws an exception on division by zero or arithmetic error (an arithmetic error is one where the minimum possible signed integer value is divided by -1).
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRDIV           8

/**
 * @def IRREM
 * \ingroup IRLanguageInstructionsArith
 *
 * Divide two values and return the remainder in the variable specified by \c result.
 *
 * Throws an exception on division by zero or arithmetic error (an arithmetic error is one where the minimum possible signed integer value is divided by -1).
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRREM           9

/**
 * @def IRAND
 * \ingroup IRLanguageInstructionsBitwise
 *
 * Bitwise AND two values and store the result within the variable specified by \c result.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRAND           10

/**
 * @def IRNEG
 * \ingroup IRLanguageInstructionsArith
 *
 * Negate a value and return the result.
 *
 * For example, if the first parameter is equal to 5, the return value will be -5.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRNEG           11

/**
 * @def IROR
 * \ingroup IRLanguageInstructionsBitwise
 *
 * Bitwise OR two values and return the result in a new temporary value.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IROR            12

/**
 * @def IRNOT
 * \ingroup IRLanguageInstructionsBitwise
 *
 * Bitwise NOT a value and return the result.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRNOT           13

/**
 * @def IRXOR
 * \ingroup IRLanguageInstructionsBitwise
 *
 * Bitwise XOR two values and return the result in a new temporary value.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRXOR           14

/**
 * @def IRSHL
 * \ingroup IRLanguageInstructionsBitwise
 *
 * Perform a bitwise left shift on two values and return the result in a new temporary value.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRSHL           15

/**
 * @def IRSHR
 * \ingroup IRLanguageInstructionsBitwise
 *
 * Perform a bitwise right shift on two values and return the result in a new temporary value.
 * This performs a signed shift on signed operators, and an unsigned shift on unsigned operands.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRSHR           16

/**
 * @def IRCONV
 * \ingroup IRLanguageInstructionsConv
 *
 * Convert the contents of a value into a new type, with optional overflow checking.\n
 * The overflow checking is performed only if the third parameter is set. \n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRType
 * @result IRVariable
 */
#define IRCONV          17

/**
 * @def IRCONVOVF
 * \ingroup IRLanguageInstructionsConv
 *
 * Convert the contents of a value into a new type, with optional overflow checking.\n
 * The overflow checking is performed only if the third parameter is set. \n
 * Throw an exception if overflow occurs. \n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRType
 * @result IRVariable
 */
#define IRCONVOVF       18

/**
 * @def IRBITCAST
 * \ingroup IRLanguageInstructionsConv
 *
 * Change the semantic of the sequence of bits stored inside the first parameter to be considered as the type of the result of the instruction.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRBITCAST   	19

/**
 * @def IRBRANCH
 * \ingroup IRLanguageInstructionsJump
 *
 * Terminate the current block by branching unconditionally to the label specified by the first parameter (i.e., \c param1).
 *
 * @param param1 IRLabel
 */
#define IRBRANCH        20

/**
 * @def IRBRANCHIF
 * \ingroup IRLanguageInstructionsJump
 *
 * Terminate the current block by branching to a specific label if the specified value is non-zero. \n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRLabel
 */
#define IRBRANCHIF      21

/**
 * @def IRBRANCHIFNOT
 * \ingroup IRLanguageInstructionsJump
 *
 * Terminate the current block by branching to a specific label if the specified value is zero. \n
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRLabel
 */
#define IRBRANCHIFNOT   22

/**
 * @def IRLABEL
 * \ingroup IRLanguageInstructionsMisc
 *
 * @param param1 IRLabel
 */
#define IRLABEL         23

/**
 * @def IRLT
 * \ingroup IRLanguageInstructionsCompare
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRLT            24

/**
 * @def IRGT
 * \ingroup IRLanguageInstructionsCompare
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRGT            25

/**
 * @def IREQ
 * \ingroup IRLanguageInstructionsCompare
 *
 * Compare two values for equality and store the result in \c result.
 * The value "0" means false and "1" means true.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IREQ            26

/**
 * @def IRCALL
 * \ingroup IRLanguageInstructionsCall
 *
 * Call the IR method specified by the first parameter.
 *
 * The first parameter is the method ID of the called method, which can be obtained as following:
 * @code
 * ir_method_t *method;
 * ...
 * IR_ITEM_VALUE methodID;
 * methodID = IRMETHOD_getMethodID(method);
 * @endcode
 *
 * @param param1 IRMethod
 * @param param2 IRFlags
 * @param callParameters (IRVariable | IRConstant | IRMethod | IRClass)*
 * @result IRVariable | epsilon
 */
#define IRCALL          27

/**
 * @def IRLIBRARYCALL
 * \ingroup IRLanguageInstructionsCall
 *
 * Call the native method provided by the virtual machine.\n
 * The body of the method specified by the first parameter is not implemented in IR; instead is stored inside the virtual machine.\n
 *
 * @param param1 IRMethod
 * @param param2 IRFlags
 * @param callParameters (IRVariable | IRConstant)*
 * @result IRVariable | epsilon
 */
#define IRLIBRARYCALL   28

/**
 * @def IRVCALL
 * \ingroup IRLanguageInstructionsCall
 *
 * Jump to the trampoline of the method specified by the first parameter (the method to jump depends on the dynamic type of the object).\n
 *
 * @param param1 IRMethod
 * @param param2 IRFlags
 * @param param3 IRConstant
 * @param callParameters (IRVariable | IRConstant)*
 * @result IRVariable | epsilon
 */
#define IRVCALL         29

/**
 * @def IRICALL
 * \ingroup IRLanguageInstructionsCall
 *
 * Call indirectly a method specified by the second parameter.
 *
 * @param param1 IRType Return type
 * @param param2 IRFPOINTER Function pointer
 * @param callParameters (IRVariable | IRConstant)*
 * @result IRVariable | epsilon
 */
#define IRICALL         30

/**
 * @def IRNATIVECALL
 * \ingroup IRLanguageInstructionsCall
 *
 * Call the native method specified by the third parameter (the second parameter is the name of this method and it is helpfull only for debugging purpose).\n
 * The difference with \ref IRLIBRARYCALL is that the body of the method can be any native method, not only from methods specified by some CIL library.
 *
 * @param param1 IRType Return type
 * @param param2 IRSTRING Method name
 * @param param3 IRFPOINTER Function pointer
 * @param callParameters (IRVariable | IRConstant)*
 * @result IRVariable | epsilon
 */
#define IRNATIVECALL    31

/**
 * @def IRNEWOBJ
 * \ingroup IRLanguageInstructionsMemoryAllocationManaged
 *
 * @param param1 IRClass Type of the object to allocate
 * @param param2 IRUint32 Extra memory to append to the object
 * @result IRVariable
 */
#define IRNEWOBJ        32

/**
 * @def IRNEWARR
 * \ingroup IRLanguageInstructionsMemoryAllocationManaged
 *
 * @param param1 IRClass Type of the array to allocate
 * @param param2 IRVariable | IRConstant. Number of elements of the array to allocate
 * @result IRVariable. Pointer to the allocated array
 */
#define IRNEWARR        33

/**
 * @def IRFREEOBJ
 * \ingroup IRLanguageInstructionsMemoryAllocationManaged
 *
 * Free the memory used by the object pointed by \c param1.
 *
 * @param param1 IRVariable
 */
#define IRFREEOBJ       34

/**
 * @def IRLOADREL
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Load the number of bytes needed by the return variable specified in the instruction from the memory location pointed by the address equal to the first parameter plus the second one.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRConstant
 * @result IRVariable
 */
#define IRLOADREL       35

/**
 * @def IRSTOREREL
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Store the value specified by the parameter \c param3 at the effective address (\c param1 + \c param2), where param1 is a pointer.
 *
 * @param param1 (IRVariable | IRConstant) Base address of the memory
 * @param param2 (IRConstant) Offset
 * @param param3 (IRVariable | IRConstant) Value to store
 */
#define IRSTOREREL      36

/**
 * @def IRGETADDRESS
 * \ingroup IRLanguageInstructionsMisc
 *
 * Get the address of the IR variable specified in \c param1 and store it to \c result.
 *
 * @param param1 IRVariable
 * @result IRVariable
 */
#define IRGETADDRESS    37

/**
 * @def IRRET
 * \ingroup IRLanguageInstructionsJump
 *
 * Return the value (if it is present) stored within the parameter \c param1 as the function's result.
 *
 * If param1 is not set, then the function is assumed to return void.
 * If the function returns a structure, this will copy the value into the memory at the structure return address.
 *
 * @param param1 IRVariable | IRConstant | epsilon
 */
#define IRRET           38

/**
 * @def IRCALLFINALLY
 * \ingroup IRLanguageInstructionsFinally
 *
 * Jump to the finally block specified by the first parameter.
 *
 * @param param1 IRLabel Identifier of the finally block to jump
 */
#define IRCALLFINALLY   39

/**
 * @def IRTHROW
 * \ingroup IRLanguageInstructionsExceptionThrow
 *
 * Throw a pointer value as an exception object.
 *
 * This can also be used to "rethrow" an object from a catch handler that is not interested in handling the exception.
 *
 * In case the instruction does not belong to the catcher block of the method that belongs to, the execution jumps to the <code> IRSTARTCATCHER </code> instruction of the same method in case it exists; in case there is no <code> IRSTARTCATCHER </code> instructions, the execution jumps to the caller unwinding the call stack.
 *
 * Finally, in case the instruction does belong to the catcher block of the method that belongs to, the execution jumps directly to the caller unwinding the call stack.
 *
 * @param param1 IRVariable
 */
#define IRTHROW         40

/**
 * @def IRSTARTFILTER
 * \ingroup IRLanguageInstructionsExceptionFilter
 *
 * Define the start of a filter.
 * Filters are embedded subroutines within functions that are used to filter exceptions in catch blocks.
 *
 * A filter subroutine takes a single argument (usually a pointer) and returns a single result (usually a boolean).
 * The filter has complete access to the local variables of the function, and can use any of them in the filtering process.
 *
 * This instruction store to the result variable a temporary value of the specified type, indicating the parameter that is supplied to the filter.
 *
 * @param param1 IRLabel
 * @result IRVariable
 */
#define IRSTARTFILTER   41

/**
 * @def IRENDFILTER
 * \ingroup IRLanguageInstructionsExceptionFilter
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRLabel
 */
#define IRENDFILTER     42

/**
 * @def IRSTARTFINALLY
 * \ingroup IRLanguageInstructionsFinally
 *
 * Start a finally block.
 *
 * @param param1 IRLabel Identifier of the finally block that this instruction starts
 */
#define IRSTARTFINALLY  43

/**
 * @def IRENDFINALLY
 * \ingroup IRLanguageInstructionsFinally
 *
 * Ends a finally block.
 *
 * @param param1 IRLabel Identifier of the finally blocks that this instruction ends
 */
#define IRENDFINALLY    44

/**
 * @def IRSTARTCATCHER
 * \ingroup IRLanguageInstructionsExceptionCatcher
 *
 * Start the catcher block. \n
 * There should be exactly one catcher block for any IR method that involves a try. \n
 * All exceptions that are thrown within the function will cause control to jump to this point. \n
 * Returns the thrown exception.
 *
 * @result IRVariable
 */
#define IRSTARTCATCHER  45

/**
 * @def IRBRANCHIFPCNOTINRANGE
 * \ingroup IRLanguageInstructionsJump
 *
 * Branch to the label specified by the third parameter (i.e., \c param3) if the program counter where an exception occurred does not fall between the first (i.e., \c param1) and second label (i.e., \c param2).
 *
 * @param param1 IRLabel
 * @param param2 IRLabel
 * @param param3 IRLabel
 */
#define IRBRANCHIFPCNOTINRANGE  46

/**
 * @def IRCALLFILTER
 * \ingroup IRLanguageInstructionsExceptionFilter
 *
 * Call a filter clause.
 *
 * @param param1 IRLabel
 * @result IRVariable
 */
#define IRCALLFILTER            47

/**
 * @def IRISNAN
 * \ingroup IRLanguageInstructionsCompare
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRISNAN         	48

/**
 * @def IRISINF
 * \ingroup IRLanguageInstructionsCompare
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRISINF         	49

/**
 * @def IRINITMEMORY
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Set the memory pointed by <code> param1 </code> to the value <code> param2 </code> for <code> param3 </code> Bytes.
 *
 * @param param1 IRVariable
 * @param param2 IRConstant
 * @param param3 IRConstant
 */
#define IRINITMEMORY    	50

/**
 * @def IRALLOCA
 * \ingroup IRLanguageInstructionsMemoryAllocation
 *
 * Allocate the number of bytes of memory specified by the first parameter from the stack. \n
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRALLOCA        	51

/**
 * @def IRMEMCPY
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Copy the <code> param3 </code> bytes of memory from the memory pointed by <code> param2 </code> to the memory pointed by <code> param1 </code>.
 *
 * It is assumed that the source and destination do not overlap.
 *
 * @param param1 IRVariable
 * @param param2 IRVariable
 * @param param3 IRVariable | IRConstant
 */
#define IRMEMCPY        	52

/**
 * @def IRMEMCMP
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Compare the first \c param3 bytes (each interpreted as unsigned char) of the memory areas \c param1 and \c param2.
 *
 * It returns an integer less than, equal to, or greater than zero if \c param1 is found, respectively, to be less than, to match, or be greater than \c param2 .
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @param param3 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRMEMCMP        	53

/**
 * @def IRCHECKNULL
 * \ingroup IRLanguageInstructionsCompare
 *
 * Check if \c param1 is NULL: if it is, then throw the null-reference exception.
 *
 * @param param1 IRVariable | IRConstant
 */
#define IRCHECKNULL     	54

/**
 * @def IRNOP
 * \ingroup IRLanguageInstructionsMisc
 *
 * No operation as well as no machine code is performed. \n
 */
#define IRNOP           	55

/**
 * @def IRCOSH
 * \ingroup IRLanguageMathInstructions
 *
 * Store the hyperbolic cosine of x to the result variable
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRCOSH          	56

/**
 * @def IRSIN
 * \ingroup IRLanguageMathInstructions
 *
 * Store the sine of x, where x is given in radians, to the result variable
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRSIN           	57

/**
 * @def IRCOS
 * \ingroup IRLanguageMathInstructions
 *
 * Store the cosine of x, where x is given in radians, to the result variable
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRCOS           	58

/**
 * @def IRACOS
 * \ingroup IRLanguageMathInstructions
 *
 * Store the arc cosine of x, where x is given in radians, to the result variable
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRACOS          	59

/**
 * @def IRSQRT
 * \ingroup IRLanguageMathInstructions
 *
 * Store the non-negative square root of x to the result variable
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRSQRT          	60

/**
 * @def IRFLOOR
 * \ingroup IRLanguageMathInstructions
 *
 * Store the largest integral value that is not greater than the first parameter of the instruction.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRFLOOR         	61

/**
 * @def IRCEIL
 * \ingroup IRLanguageMathInstructions
 *
 * Store the smallest integral value that is not less than the first parameter.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRCEIL          	62

/**
 * @def IRPOW
 * \ingroup IRLanguageMathInstructions
 *
 * Store the value of the first parameter raised to the power of the second one.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRPOW         		63

/**
 * @def IREXP
 * \ingroup IRLanguageMathInstructions
 *
 * Store the value of e (the base of natural logarithms) raised to the power of first parameter.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IREXP         		64

/**
 * @def IRLOG10
 * \ingroup IRLanguageMathInstructions
 *
 * Store the base 10 logarithm of param1.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRLOG10        		65

/**
 * @def IRALLOC
 * \ingroup IRLanguageInstructionsMemoryAllocation
 *
 * Allocate the number of bytes of memory specified by the first parameter (i.e., \c param1) from the heap.
 *
 * @param param1 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRALLOC         	66

/**
 * @def IRALLOCALIGN
 * \ingroup IRLanguageInstructionsMemoryAllocation
 *
 * Allocate the number of bytes of memory specified by the first parameter (i.e., \c param1) from the heap.
 *
 * The address of the allocated memory will be a multiple of \c param2, which must be a power of two and a multiple of sizeof(IRMPOINTER).
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRALLOCALIGN    	67

/**
 * @def IRREALLOC
 * \ingroup IRLanguageInstructionsMemoryAllocation
 *
 * Reallocate the memory pointed by the first parameter to point to a new memory of size specified by the second parameter (i.e., \c param2).
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRREALLOC         	68

/**
 * @def IRCALLOC
 * \ingroup IRLanguageInstructionsMemoryAllocation
 *
 * Allocate memory for an array of \c param1 elements of \c param2 bytes each and returns a pointer to the allocated memory.
 * The memory is set to zero.
 *
 * If \c param1 or \c param2 is 0, then \c result is set to 0 or a unique pointer value that can later be successfully passed to \ref IRFREE .
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRCALLOC         	69

/**
 * @def IRFREE
 * \ingroup IRLanguageInstructionsMemoryAllocation
 *
 * Free the memory pointed by the first parameter (i.e., param1)
 *
 * @param param1 IRVariable | IRConstant
 */
#define IRFREE          	70

/**
 * @def IREXIT
 * \ingroup IRLanguageInstructionsJump
 *
 * Force the execution of the program to quit
 * @param param1 IRConstant | IRVariable
 */
#define IREXIT      		71

/**
 * @def IRPHI
 * \ingroup IRLanguageInstructionsMisc
 *
 * Identifies the \c phi instruction present when the method code is in Static Single Assignment form.
 * The first parameter is used for keeping track of the ID of the original variable the \c phi is creating one alias.
 * The call parameters are all the variables this \c phi may read.
 *
 * @param param1 IRConstant
 * @param callParameters IRVariable +
 * @return IRVariable
 */
#define IRPHI 			72

/**
 * @def IRARRAYLENGTH
 * \ingroup IRLanguageInstructionsArrayMemoryOperations
 *
 * Store the length of the array given as first parameter to the result
 *
 * @param param1 IRVariable | IRConstant
 * @return IRVariable
 */
#define IRARRAYLENGTH 		73

/**
 * @def IRSTRINGLENGTH
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Store the length of the (char *) string (i.e., the C string, not the C\# string) given as first parameter to the result.
 *
 * @param param1 IRVariable | IRConstant
 * @return IRVariable
 */
#define IRSTRINGLENGTH 		74

/**
 * @def IRSTRINGCMP
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Compare the two strings (i.e., the C string, not the C\# string) \c param1 and \c param2 .
 *
 * It returns an integer less than, equal to, or greater than zero if \param1 is found, respectively, to be less than, to match, or be greater than \c param2 .
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @return IRVariable
 */
#define IRSTRINGCMP 		75

/**
 * @def IRSTRINGCHR
 * \ingroup IRLanguageInstructionsMemoryOperations
 *
 * Return a pointer to the first occurrence of the character \c param2 in the string (i.e., the C string, not the C\# string) pointed by \c param1 .
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @return IRVariable
 */
#define IRSTRINGCHR 		76

/**
 * @def IRSIZEOF
 * \ingroup IRLanguageInstructionsMisc
 *
 * Store the number of bytes occupied by the variable given as first parameter.
 *
 * In case the variable points to a managed object, it returns the number of bytes of the object.
 *
 * @param param1 IRVariable | IRConstant
 * @return IRVariable
 */
#define IRSIZEOF 		    77

/**
 * @def IRDIV_NOEXCEPTION
 * \ingroup IRLanguageInstructionsArith
 *
 * Divide two values (\c param1 and \c param2) and store the quotient in \c result.
 *
 * For division by zero or arithmetic error the behaviour is unspecified.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRDIV_NOEXCEPTION	78

/**
 * @def IRREM_NOEXCEPTION
 * \ingroup IRLanguageInstructionsArith
 *
 * Divide two values and return the remainder in the variable specified by \c result.

 * For division by zero or arithmetic error the behaviour is unspecified.
 *
 * @param param1 IRVariable | IRConstant
 * @param param2 IRVariable | IRConstant
 * @result IRVariable
 */
#define IRREM_NOEXCEPTION	79

/**
 * @def IRFENCE
 * \ingroup IRLanguageInstructionsMisc
 *
 * A memory fence. This is an instruction for ordering other memory operations.
 *
 * The first parameter decides the type of fence. 
 * 
 * Options are:
 * 0 = NotAtomic    
 * 1 = Unordered   
 * 2 = Monotonic   
 * 3 = Acquire     
 * 4 = Release     
 * 5 = AcquireRelease  
 * 6 = SequentiallyConsistent 
 *
 * @param param1 IRConstant
 */
#define IRFENCE       	    80

/**
 * @def IREXITNODE
 * \ingroup IRLanguageInstructionsMisc
 *
 * Identifies the single exit node of the CFG.
 */
#define IREXITNODE      	81

/**
 * @def IROFFSET
 * \ingroup IRLanguageDataTypes
 *
 * Identifies a IR variable
 */
#define IROFFSET                1

/**
 * @def IRTYPE
 * \ingroup IRLanguageDataTypes
 *
 * Represent a type of the IR language.
 * A IR variable of this type (internal_type of ir_item_t will be equal to IRTYPE) will store a value that identify a IR type.\n
 * For example, if the variable has the value equal to \ref IRINT8 (i.e. ir_item_t.value == \ref IRINT8) it means that identify the \ref IRINT8 type.
 */
#define IRTYPE                  2

/**
 * @def IRLABELITEM
 * \ingroup IRLanguageDataTypes
 *
 * Represent labels inside the IR methods.
 * A IR variable of this kind (i.e., ir_item_t.type == IRLABELITEM, ir_item_t.internal_type == IRLABELITEM, ir_item_t.value == "number of the label (ID)") represents a point of the Control Flow Graph where branch instructions can jump to it.
 */
#define IRLABELITEM             3

/**
 * @def IRINT8
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent a integer IR variable of bit width equal to 8 bits.
 */
#define IRINT8                  4

/**
 * @def IRINT16
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent a integer IR variable of bit width equal to 16 bits.
 */
#define IRINT16                 5

/**
 * @def IRINT32
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent a integer IR variable of bit width equal to 32 bits.
 */
#define IRINT32                 6

/**
 * @def IRINT64
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent a integer IR variable of bit width equal to 64 bits.
 */
#define IRINT64                 7

/**
 * @def IRNINT
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent a integer IR variable where the size depends on the underlying platform (sizeof(\ref JITNINT) gives you the number of Bytes of a IRNINT variable).
 */
#define IRNINT                  8

/**
 * @def IRUINT8
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent an unsigned integer IR variable of bit width equal to 8 bits.
 */
#define IRUINT8                 9

/**
 * @def IRUINT16
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent an unsigned integer IR variable of bit width equal to 16 bits.
 */
#define IRUINT16                10

/**
 * @def IRUINT32
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent an unsigned integer IR variable of bit width equal to 32 bits.
 */
#define IRUINT32                11

/**
 * @def IRUINT64
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent an unsigned integer IR variable of bit width equal to 64 bits.
 */
#define IRUINT64                12

/**
 * @def IRNUINT
 * \ingroup IRLanguageIntDataTypes
 *
 * Represent an unsigned integer IR variable where the size depends on the underlying platform (sizeof(\ref JITNUINT) gives you the number of Bytes of a IRNUINT variable).
 */
#define IRNUINT                 13

/**
 * @def IRFLOAT32
 * \ingroup IRLanguageFloatDataTypes
 *
 * Represent a floating point IR variable of bit width equal to 32 bits.
 */
#define IRFLOAT32               14

/**
 * @def IRFLOAT64
 * \ingroup IRLanguageFloatDataTypes
 *
 * Represent a floating point IR variable of bit width equal to 64 bits.
 */
#define IRFLOAT64               15

/**
 * @def IRNFLOAT
 * \ingroup IRLanguageFloatDataTypes
 *
 * Represent a floating point IR variable where the size depends on the underlying platform (sizeof(\ref JITNFLOAT) gives you the number of Bytes of a IRNFLOAT variable).
 */
#define IRNFLOAT                16

/**
 * @def IRVOID
 * \ingroup IRLanguageDataTypes
 *
 * Represent the void type which means "no type"
 */
#define IRVOID                  17

/**
 * @def IRCLASSID
 * \ingroup IRLanguagePointerDataTypes
 *
 * Represent the identificator of an application class (e.g. CIL class).
 */
#define IRCLASSID               18

/**
 * @def IRMETHODID
 * \ingroup IRLanguagePointerDataTypes
 *
 * Represent the identificator of an application method (e.g. CIL method, IR method).\n
 * A IR variable (or constant) of this type (stored inside the internal_type field of ir_item_t) will store the identificator of a IR method.
 */
#define IRMETHODID              19

/**
 * @def IRMETHODENTRYPOINT
 * \ingroup IRLanguagePointerDataTypes
 *
 * A IR variable (or constant) of this kind reports the first address where the assembly of a IR method is stored.\n
 * If that IR method is not yet compiled, it reports the address where the assembly of the trampoline to that method is stored.
 */
#define IRMETHODENTRYPOINT      20

/**
 * @def IRMPOINTER
 * \ingroup IRLanguagePointerDataTypes
 *
 * Pointer to objects or fields of objects (Managed pointer).\n
 * The difference with \ref IROBJECT is that IRMPOINTER can additionally point (i.e., store the address) to a field of an object rather than pointing only to the first byte of the object as \ref IROBJECT does.
 */
#define IRMPOINTER              21
#define IRTPOINTER              22

/**
 * @def IRFPOINTER
 * \ingroup IRLanguagePointerDataTypes
 *
 * Function pointer
 */
#define IRFPOINTER              23

/**
 * @def IROBJECT
 * \ingroup IRLanguagePointerDataTypes
 *
 * IROBJECT represents Application object (instances of application classes)
 */
#define IROBJECT                24

/**
 * @def IRVALUETYPE
 * \ingroup IRLanguageDataTypes
 *
 * IRVALUETYPE represents user data structure.\n
 * They are usually stored within the stack instead of the heap (but this is not a constraint).
 */
#define IRVALUETYPE             25

/**
 * @def IRSTRING
 * \ingroup IRLanguagePointerDataTypes
 *
 * Represent strings of the compiler. Basically they are pointer to char.\n
 * Notice that they do not represent Application strings (e.g. CIL strings) which are of type \ref IROBJECT.
 */
#define IRSTRING                26

/**
 * @def IRSIGNATURE
 * \ingroup IRLanguagePointerDataTypes
 *
 * A IR variable (or constant) of this type stores the memory address of the <code> \ref t_ir_signature </code>.
 */
#define IRSIGNATURE             27

/**
 * @def IRSYMBOL
 * \ingroup IRLanguageDataTypes
 *
 * Identifies a IR Symbol
 */
#define IRSYMBOL                28

/**
 * @def IRGLOBAL
 * \ingroup IRLanguageDataTypes
 *
 * Identifies a IR global
 */
#define IRGLOBAL                29

/**
 * @def NOPARAM
 * \ingroup IRLanguageDataTypes
 *
 * Represent the absence of parameters.
 */
#define NOPARAM                 -1

#endif
