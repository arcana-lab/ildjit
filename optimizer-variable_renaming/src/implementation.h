/*
 * Copyright (C) 2009-2010  Mastrandrea Daniele
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
 * @file implementation.h
 * @brief Macro collection, in order to hide actual implementation details
 * @author Mastrandrea Daniele
 * @date 2010
 * @note These macros refer to ILDJIT v0.1.4
 **/

#ifndef IMPLEMENTATION_MACROS_H
#define IMPLEMENTATION_MACROS_H

/**
 * @defgroup ImplementationMacros
 * Macros to hide implementation details
 **/

/**
 * @def METH_NAME(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the name of the method.
 **/
#define METH_NAME(method) method->getCompleteName(method)

/**
 * @def METH_N_INST(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the number of instructions in the method.
 **/
#define METH_N_INST(method) method->instructionsTop
//#define METH_N_INST(method) method->getInstructionsNumber(method)

/**
 * @def METH_MAX_VAR(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the number of variable used.
 **/
#define METH_MAX_VAR(method) IRMETHOD_getNumberOfVariablesNeededByMethod(method)

/**
 * @def METH_MAX_VAR_SET(method, new_value)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new number of variable used.
 **/
#define METH_MAX_VAR_SET(method, new_value) IRMETHOD_setNumberOfVariablesNeededByMethod(method, new_value)

/**
 * @def METH_N_PAR(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the number of parameters used.
 **/
#define METH_N_PAR(method) IRMETHOD_getMethodParametersNumber(method)

/**
 * @def METH_N_LOCAL(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the number of locals used.
 **/
#define METH_N_LOCAL(method) IRMETHOD_getMethodLocalsNumber(method)

/**
 * @def METH_N_TEMP(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the number of tempraries used.
 **/
#define METH_N_TEMP(method) (METH_MAX_VAR(method) - METH_N_PAR(method) - METH_N_LOCAL(method))

/**
 * @def INST_AT(method, instructionID)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the instruction with a specific ID.
 **/
#define INST_AT(method, instructionID) IRMETHOD_getInstructionAtPosition(method, instructionID)

/**
 * @def INST_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the IR-type of an instruction inside the method.
 **/
#define INST_TYPE(method, instruction) method->getInstructionType(method, instruction)

/**
 * @def INST_N_PAR(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the number of parameters of an instruction inside the method.
 **/
#define INST_N_PAR(method, instruction) IRMETHOD_getInstructionParametersNumber(instruction)

/**
 * @def PREV_INST(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the instruction stored just before the instruction of interest.
 **/
#define PREV_INST(method, instruction) IRMETHOD_getPrevInstruction(method, instruction)

/**
 * @def NEXT_INST(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the instruction stored just after the instruction of interest.
 **/
#define NEXT_INST(method, instruction) IRMETHOD_getNextInstruction(method, instruction)

/**
 * @def GET_PREDECESSOR(method, instruction, prevPredecessor)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the predecessor of an instruction, according to the previously returned (if one).
 **/
#define GET_PREDECESSOR(method, instruction, prevPredecessor) method->getPredecessorInstruction(method, instruction, prevPredecessor)

/**
 * @def GET_SUCCESSOR(method, instruction, prevSuccessor)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the successor of an instruction, according to the previously returned (if one).
 **/
#define GET_SUCCESSOR(method, instruction, prevSuccessor) method->getSuccessorInstruction(method, instruction, prevSuccessor)

/**
 * @def GET_BB(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the basic block that the instruction belongs to.
 **/
#define GET_BB(method, instruction) method->getBasicBlockFromInstruction(method, instruction)

/**
 * @def FIRST_IN_BB(method, basic_block)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the first instruction in a basic block.
 **/
#define FIRST_IN_BB(method, basic_block) INST_AT(method, basic_block->startInst)

/**
 * @def LAST_IN_BB(method, basic_block)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the last instruction in a basic block.
 **/
#define LAST_IN_BB(method, basic_block) INST_AT(method, basic_block->endInst)

/**
 * @def NEW_INST(method)
 * @ingroup ImplementationMacros
 * @brief A macro that creates a new instruction, storing it at the end of the method.
 **/
#define NEW_INST(method) method->newInstruction(method)

/**
 * @def CLONE_INST(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that creates a new instruction, cloning an old one, storing it at the end of the method.
 **/
#define CLONE_INST(method, instruction) method->cloneInstruction(method, instruction)

/**
 * @def DEL_INST(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that deletes an existing instruction.
 **/
#define DEL_INST(method, instruction) method->deleteInstruction(method, instruction)

/**
 * @def NEW_INST_AFTER(method, previous)
 * @ingroup ImplementationMacros
 * @brief A macro that creates a new instruction, storing it just after another one.
 **/
#define NEW_INST_AFTER(method, previous) method->newInstructionAfter(method, previous)

/**
 * @def MOVE_INST_AFTER(method, instruction, previous)
 * @ingroup ImplementationMacros
 * @brief A macro that moves an existing instruction just after another one.
 **/
#define MOVE_INST_AFTER(method, instruction, previous) method->moveInstructionAfter(method, instruction, previous)

/**
 * @def NEW_INST_BEFORE(method, following)
 * @ingroup ImplementationMacros
 * @brief A macro that creates a new instruction, storing it just before another one.
 **/
#define NEW_INST_BEFORE(method, following) method->newInstructionBefore(method, following)

/**
 * @def MOVE_INST_BEFORE(method, instruction, following)
 * @ingroup ImplementationMacros
 * @brief A macro that moves an existing instruction just before another one.
 **/
#define MOVE_INST_BEFORE(method, instruction, following) method->moveInstructionBefore(method, instruction, following)

/**
 * @def INST_TYPE_SET(method, instruction, new_type)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new type of an instruction.
 **/
#define INST_TYPE_SET(method, instruction, new_type) IRMETHOD_setInstructionType(instruction, new_type);

/**
 * @def LABEL_AT(method, labelID)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the instruction defining a label ID.
 **/
#define LABEL_AT(method, labelID) method->getLabelInstruction(method, labelID)

/**
 * @def NEW_LABEL(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns an available label ID.
 **/
#define NEW_LABEL(method) method->newLabelID(method)

/**
 * @def IS_A_JUMP(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns true if the instruction is a jump.
 **/
#define IS_A_JUMP(method, instruction) method->isAJumpInstruction(method, instruction)

/**
 * @def IS_A_PREDOM(method, dominator, dominated)
 * @ingroup ImplementationMacros
 * @brief A macro that returns true if the first instruction (pre)dominates the second.
 **/
#define IS_A_PREDOM(method, dominator, dominated) method->isAPredominator(method, dominator, dominated)

/**
 * @def IS_A_TEMPORARY(method, variableID)
 * @ingroup ImplementationMacros
 * @brief A macro that returns true if the variable is a temporary.
 **/
#define IS_A_TEMPORARY(method, variableID) (variableID >= METH_N_PAR(method) + METH_N_LOCAL(method)) ? JITTRUE : JITFALSE

/**
 * @def LOOPS(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the list of the loops.
 **/
#define LOOPS(method) method->loop

/**
 * @def PREDOM_TREE(method)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the predominator tree.
 **/
#define PREDOM_TREE(method) method->preDominatorTree

/**
 * @def PARENT_LOOP(method, childLoop)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the immediate parent of a loop.
 **/
#define PARENT_LOOP(method, childLoop) IRMETHOD_getParentLoop(method, childLoop)

/**
 * @def LOOP_INSTRUCTIONS(method, loop)
 * @ingroup ImplementationMacros
 * @brief A macro that returns a list of the instructions in a loop.
 **/
#define LOOP_INSTRUCTIONS(method, loop) IRMETHOD_getLoopInstructions(method, loop)

/**
 * @def PAR_1(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the first parameter of an instruction.
 **/
#define PAR_1(method, instruction) IRMETHOD_getInstructionParameter1(instruction)

/**
 * @def PAR_1_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new values for all the fields of the first parameter of an instruction.
 **/
#define PAR_1_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype) method->setInstrPar1(inst, value, fvalue, type, internal_type, is_byref, iltype);

/**
 * @def PAR_1_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the type of the first parameter of an instruction.
 **/
#define PAR_1_TYPE(method, instruction) IRMETHOD_getInstructionParameter1Type(instruction)

/**
 * @def PAR_1_INT_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the internal type of the first parameter of an instruction.
 **/
#define PAR_1_INT_TYPE(method, instruction) method->getInstrPar1IntType(instruction)

/**
 * @def PAR_1_VALUE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the value of the first parameter of an instruction.
 **/
#define PAR_1_VALUE(method, instruction) IRMETHOD_getInstructionParameter1Value(instruction)

/**
 * @def PAR_1_VALUE_SET(method, instruction, new_value)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new value of the first parameter of an instruction.
 **/
#define PAR_1_VALUE_SET(method, instruction, new_value) IRMETHOD_setInstructionParameter1Value(method, instruction, new_value)

/**
 * @def PAR_2(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the second parameter of an instruction.
 **/
#define PAR_2(method, instruction) IRMETHOD_getInstructionParameter2(instruction)

/**
 * @def PAR_2_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new values for all the fields of the second parameter of an instruction.
 **/
#define PAR_2_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype) method->setInstrPar3(inst, value, fvalue, type, internal_type, is_byref, iltype);

/**
 * @def PAR_2_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the type of the second parameter of an instruction.
 **/
#define PAR_2_TYPE(method, instruction) IRMETHOD_getInstructionParameter2Type(instruction)

/**
 * @def PAR_2_INT_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the internal type of the second parameter of an instruction.
 **/
#define PAR_2_INT_TYPE(method, instruction) method->getInstrPar2IntType(instruction)

/**
 * @def PAR_2_VALUE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the value of the second parameter of an instruction.
 **/
#define PAR_2_VALUE(method, instruction) IRMETHOD_getInstructionParameter2Value(instruction)

/**
 * @def PAR_2_VALUE_SET(method, instruction, new_value)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new value of the second parameter of an instruction.
 **/
#define PAR_2_VALUE_SET(method, instruction, new_value) IRMETHOD_setInstructionParameter2Value(method, instruction, new_value)

/**
 * @def PAR_3(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the third parameter of an instruction.
 **/
#define PAR_3(method, instruction) IRMETHOD_getInstructionParameter3(instruction)

/**
 * @def PAR_3_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new values for all the fields of the third parameter of an instruction.
 **/
#define PAR_3_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype) method->setInstrPar3(inst, value, fvalue, type, internal_type, is_byref, iltype);

/**
 * @def PAR_3_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the type of the third parameter of an instruction.
 **/
#define PAR_3_TYPE(method, instruction) IRMETHOD_getInstructionParameter3Type(instruction)

/**
 * @def PAR_3_INT_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the internal type of the third parameter of an instruction.
 **/
#define PAR_3_INT_TYPE(method, instruction) method->getInstrPar3IntType(instruction)

/**
 * @def PAR_3_VALUE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the value of the third parameter of an instruction.
 **/
#define PAR_3_VALUE(method, instruction) IRMETHOD_getInstructionParameter3Value(instruction)

/**
 * @def PAR_3_VALUE_SET(method, instruction, new_value)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new value of the third parameter of an instruction.
 **/
#define PAR_3_VALUE_SET(method, instruction, new_value) IRMETHOD_setInstructionParameter3Value(method, instruction, new_value)

/**
 * @def PAR_4(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the fourth parameter of an instruction.
 **/
#define PAR_4(method, instruction) method->getInstrPar4(instruction)

/**
 * @def PAR_4_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new values for all the fields of the fourth parameter of an instruction.
 **/
#define PAR_4_SET(method, inst, value, fvalue, type, internal_type, is_byref, iltype) method->setInstrPar4(inst, value, fvalue, type, internal_type, is_byref, iltype);

/**
 * @def PAR_4_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the type of the fourth parameter of an instruction.
 **/
#define PAR_4_TYPE(method, instruction) method->getInstrPar4Type(instruction)

/**
 * @def PAR_4_INT_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the internal type of the fourth parameter of an instruction.
 **/
#define PAR_4_INT_TYPE(method, instruction) method->getInstrPar4IntType(instruction)

/**
 * @def PAR_4_VALUE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the value of the fourth parameter of an instruction.
 **/
#define PAR_4_VALUE(method, instruction) method->getInstrPar4Value(instruction)

/**
 * @def PAR_4_VALUE_SET(method, instruction, new_value)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new value of the fourth parameter of an instruction.
 **/
#define PAR_4_VALUE_SET(method, instruction, new_value) method->setInstrPar4Value(instruction, new_value)

/**
 * @def RES(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the result of an instruction.
 **/
#define RES(method, instruction) IRMETHOD_getInstructionResult(instruction)

/**
 * @def RES_TYPE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the type of the result of an instruction.
 **/
#define RES_TYPE(method, instruction) IRMETHOD_getInstructionResult(instruction)->type

/**
 * @def RES_VALUE(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the value of the result of an instruction.
 **/
#define RES_VALUE(method, instruction) IRMETHOD_getInstructionResult(instruction)->value.v

/**
 * @def RES_VALUE_SET(method, instruction, new_value)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new value of the result of an instruction.
 **/
#define RES_VALUE_SET(method, instruction, new_value) IRMETHOD_setInstructionParameterResultValue(method, instruction, new_value)

/**
 * @def CALL_PAR_NUM(method, instruction)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the number of call parameters of an instruction.
 **/
#define CALL_PAR_NUM(method, instruction) IRMETHOD_getInstructionCallParametersNumber(instruction)

/**
 * @def CALL_PAR_TYPE(method, instruction, nth)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the type of the n-th call parameter of an instruction.
 **/
#define CALL_PAR_TYPE(method, instruction, nth) IRMETHOD_getInstructionCallParameterType(instruction, nth)

/**
 * @def CALL_PAR_VALUE(method, instruction, nth)
 * @ingroup ImplementationMacros
 * @brief A macro that returns the value of the n-th call parameter of an instruction.
 **/
#define CALL_PAR_VALUE(method, instruction, nth) IRMETHOD_getInstructionCallParameterValue(instruction, nth)

/**
 * @def CALL_PAR_VALUE_SET(method, instruction, nth, new_value)
 * @ingroup ImplementationMacros
 * @brief A macro that sets the new value of the n-th call parameter of an instruction.
 **/
#define CALL_PAR_VALUE_SET(method, instruction, nth, new_value) IRMETHOD_setInstructionCallParameterValue(method, instruction, nth, new_value)

#endif /* IMPLEMENTATION_MACROS_H */
