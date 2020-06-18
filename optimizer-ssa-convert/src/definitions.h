/*
 * Copyright (C) 2007-2010  Michele Melchiori
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
#ifndef SSA_CONVERT_DEFINITIONS_H
#define SSA_CONVERT_DEFINITIONS_H

/**
 * @defgroup SSA_CONVERT_HELPERS Set of definitions used as helper functions in the code
 */

/**
 * @defgroup SSA_CONVERT_STRUCTS Set of definitions of structures used in the plugin
 */
/**
 * @defgroup SSA_CONVERT_OPTIONS Compilation options for this plugin
 */

/**
 * @def NUM_VARS
 * @ingroup SSA_CONVERT_HELPERS
 * The number of variables used in this method.
 */
#define NUM_VARS(method) IRMETHOD_getNumberOfVariablesNeededByMethod(method)

/**
 * @def LIVENESS_BLOCKS_NUMBER
 * @ingroup SSA_CONVERT_HELPERS
 * Number of blocks in the array used for computing liveness informations.
 */
#define LIVENESS_BLOCKS_NUMBER(method) ((NUM_VARS(method) / (sizeof(JITNUINT) * 8)) + 1)

/**
 * @def CHECK_INSTR_RES_IS_VAR(meth, instr, var)
 * @ingroup SSA_CONVERT_HELPERS
 * Combines the three conditions necessary to verify if the result of the
 * instruction is exactly the variable passed as last parameter
 *
 * @param instr pointer to the instruction we want to consider
 * @param var   the id of the variable we are looking for
 */
#define CHECK_INSTR_RES_IS_VAR(instr, var) ( \
	IRMETHOD_getInstructionResult(instr)      != NULL     && \
	IRMETHOD_getInstructionResultType(instr)  == IROFFSET && \
	IRMETHOD_getInstructionResultValue(instr) == var         \
)

/**
 * @def CHECK_INSTR_PAR1_IS_VAR(instr, var)
 * @ingroup SSA_CONVERT_HELPERS
 * Combines the three conditions necessary to verify if the first parameter of
 * the instruction is exactly the variable passed as last parameter
 *
 * @param instr pointer to the instruction we want to consider
 * @param var   the id of the variable we are looking for
 */
#define CHECK_INSTR_PAR1_IS_VAR(instr, var) ( \
	IRMETHOD_getInstructionParameter1(instr)      != NULL     &&    \
	IRMETHOD_getInstructionParameter1Type(instr)  == IROFFSET &&    \
	IRMETHOD_getInstructionParameter1Value(instr) == var            \
)

/**
 * @def CHECK_INSTR_PAR2_IS_VAR(instr, var)
 * @ingroup SSA_CONVERT_HELPERS
 * Combines the three conditions necessary to verify if the second parameter of
 * the instruction is exactly the variable passed as last parameter
 *
 * @param instr pointer to the instruction we want to consider
 * @param var   the id of the variable we are looking for
 */
#define CHECK_INSTR_PAR2_IS_VAR(instr, var) ( \
	IRMETHOD_getInstructionParameter2(instr)      != NULL     &&    \
	IRMETHOD_getInstructionParameter2Type(instr)  == IROFFSET &&    \
	IRMETHOD_getInstructionParameter2Value(instr) == var            \
)

/**
 * @def CHECK_INSTR_PAR3_IS_VAR(instr, var)
 * @ingroup SSA_CONVERT_HELPERS
 * Combines the three conditions necessary to verify if the third parameter of
 * the instruction is exactly the variable passed as last parameter
 *
 * @param instr pointer to the instruction we want to consider
 * @param var   the id of the variable we are looking for
 */
#define CHECK_INSTR_PAR3_IS_VAR(instr, var) ( \
	IRMETHOD_getInstructionParameter3(instr)      != NULL     &&    \
	IRMETHOD_getInstructionParameter3Type(instr)  == IROFFSET &&    \
	IRMETHOD_getInstructionParameter3Value(instr) == var            \
)

/**
 * @struct BasicBlockInfo
 * @ingroup SSA_CONVERT_STRUCTS
 *
 * This structure is defined for making a link between basic blocks of the
 * method and their dominance frontier. The latter are defined as a \c XanList
 * of pointers to the basic blocks that are in the dominance frontier of the one
 * pointed by \c basicBlock.
 *
 * This structure has also two pointers at \c ir_instruction_t that identifies
 * the first and the last instruction of the basic block. They are used with the
 * objective to not lose these points of the code when inserting PHI functions.
 */
typedef struct {
    IRBasicBlock*       basicBlock;   ///< the basic block we are considering
    ir_instruction_t* startInst;    ///< pointer to the first instruction of the basic block
    ir_instruction_t* endInst;      ///< pointer to the last instruction of the basic block
    XanList*          domFront;     ///< the dominance frontier of this basic block: one XanList of pointers to \c BasicBlockInfo
    XanList*          predList;     ///< the list of predecessors of this basic block. Used when computing the dominance tree
    XanList*          succList;     ///< the list of successors of this basic block. Used when computing the dominance tree
} BasicBlockInfo;

/**
 * @struct VarInfo
 * @ingroup SSA_CONVERT_STRUCTS
 *
 * This struct is able to link one variable whit its type and all its definition
 * points and use points.
 */
typedef struct {
    ir_item_t*      var;            ///< structure that is able to contain the basic variable informations
    XanList*        defPoints;      ///< list of pointers to all instructions that defines the variable with id = varId
    XanStack*       renameStack;    ///< stack used during the rename phase. \c NULL means that no definitions of the variable \c var are yet reached, not \c NULL but empty means that only one definition of \c var is reached
} VarInfo;

/**
 * @struct GlobalObjects
 * @ingroup SSA_CONVERT_STRUCTS
 *
 * This structure keeps informations on all global variables needed by the
 * plugin functions. It was created in order to reduce the number of parameters
 * passed to these functions.
 */
typedef struct {
    ir_method_t*     method;             ///< pointer to the method we are optimizing
    JITNUINT*        varLiveAcrossBB;    ///< bitset that identifies the vars that are live_in in some basic block
    JITNUINT         varLiveAcrossBBLen; ///< length of the array bitset
    XanList*         varInfoList;        ///< the list containing info on the def points for each var
    XanNode*         root;               ///< the root of dominance tree, it is a tree of \c BasicBlockInfo*
    BasicBlockInfo** bbInfos;            ///< pointer to the array containing the pointers to all \c BasicBlockInfo structures
    JITNUINT         bbInfosLen;         ///< size of the previous array
} GlobalObjects;

/**
 * @ingroup SSA_CONVERT_OPTIONS
 *
 * type: boolean
 * If equal to \c JITTRUE the plugin continue using the lists of basic block
 * predecessors / successors created during the computation of the dominance
 * frontier.
 * If equal to \c JITFALSE these lists are destroyed when no longer needed and
 * so the memory requirements will be lower.
 */
#define SSA_CONVERT_KEEP_PRED_SUCC_LISTS JITFALSE

#endif
