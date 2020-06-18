/*
 * Copyright (C) 2012  Campanoni Simone
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
 * @file codetool_types.h
 */
#ifndef CODETOOL_TYPES_H
#define CODETOOL_TYPES_H

/**
 * @def DDG_COMPUTER
 * \ingroup Codetools
 * @brief Data dependence graph computation
 *
 * It computes the data dependence graph
 */
#define DDG_COMPUTER                            0x1

/**
 * @def CDG_COMPUTER
 * \ingroup Codetools
 * @brief Control dependence graph computation
 *
 * It computes the control dependence graph
 */
#define CDG_COMPUTER                            0x2

/**
 * @def LIVENESS_ANALYZER
 * \ingroup Codetools
 * @brief Variable liveness analysis
 *
 * It computes the variable liveness analysis
 */
#define LIVENESS_ANALYZER                       0x4

/**
 * @def REACHING_EXPRESSIONS_ANALYZER
 * \ingroup Codetools
 * @brief Reaching experssions analysis
 *
 * It computes the Reaching experssions analysis
 */
#define REACHING_EXPRESSIONS_ANALYZER           0x8

/**
 * @def REACHING_DEFINITIONS_ANALYZER
 * \ingroup Codetools
 * @brief Reaching definitions analysis
 *
 * It computes the reaching definitions analysis
 */
#define REACHING_DEFINITIONS_ANALYZER           0x10

/**
 * @def AVAILABLE_EXPRESSIONS_ANALYZER
 * \ingroup Codetools
 * @brief Available expressions analysis
 *
 * It computes the available expressions analysis
 */
#define AVAILABLE_EXPRESSIONS_ANALYZER          0x20

/**
 * @def DEADCODE_ELIMINATOR
 * \ingroup Codetools
 * @brief Deadcode code transformation
 *
 * It performs the common dead-code optimization.
 */
#define DEADCODE_ELIMINATOR                     0x40

/**
 * @def IR_CHECKER
 * \ingroup Codetools
 * @brief Check IR code
 *
 * It checks the correctness of IR code.
 */
#define IR_CHECKER                              0x80

/**
 * @def LOOP_INVARIANTS_IDENTIFICATION
 * \ingroup Codetools
 * @brief Loop invariants identification
 *
 * This type of codetools identify both instructions and variables that are invariants with respect to a given loop.
 *
 */
#define LOOP_INVARIANTS_IDENTIFICATION          0x100

/**
 * @def METHOD_PRINTER
 * \ingroup Codetools
 * @brief Print IR methods
 *
 * This job is to print, or to dump, IR methods
 */
#define METHOD_PRINTER                          0x200

/**
 * @def SCALARIZATION
 * \ingroup Codetools
 * @brief Transform memory locations into scalar variables
 *
 * Transform memory locations into scalar variables.
 */
#define SCALARIZATION                           0x400

/**
 * @def COMMON_SUBEXPRESSIONS_ELIMINATOR
 * \ingroup Codetools
 * @brief Common subexpressions elimination
 *
 * This codetools eliminate subexpressions that are shared between IR instructions.
 */
#define COMMON_SUBEXPRESSIONS_ELIMINATOR        0x800

/**
 * @def CONSTANT_PROPAGATOR
 * \ingroup Codetools
 * @brief Constant propagation code transformation
 *
 * It performs the constant propagation code transformation
 */
#define CONSTANT_PROPAGATOR                     0x1000

/**
 * @def INDUCTION_VARIABLES_IDENTIFICATION
 * \ingroup Codetools
 * @brief Induction variables identification
 *
 * This type of codetools identify variables that are induction with respect to a given loop.
 *
 */
#define INDUCTION_VARIABLES_IDENTIFICATION      0x2000

/**
 * @def BASIC_BLOCK_IDENTIFIER
 * \ingroup Codetools
 * @brief Basic blocks analysis
 *
 * This job identifies basic blocks within IR methods
 */
#define BASIC_BLOCK_IDENTIFIER                  0x4000

/**
 * @def ESCAPES_ANALYZER
 * @ingroup Codetools
 * @brief Escapes analysis
 *
 * Analyse variables to identify those that escape beyond their scope.  These are variables who are the source to an @ref IRGETADDRESS instruction.
 */
#define ESCAPES_ANALYZER                        0x8000

/**
 * @def INSTRUCTIONS_SCHEDULER
 * @ingroup Codetools
 * @brief Schedule instructions
 *
 * Schedule instructions within a method to minimize the number of static branches.
 */
#define INSTRUCTIONS_SCHEDULER                  0x10000

/**
 * @def BRANCH_PREDICTOR
 * @ingroup Codetools
 * @brief Make static predictions of branch directions
 *
 * Analyse instructions within each basic block to make static predictions (i.e., probabilities) about the direction each branch will go.
 */
#define BRANCH_PREDICTOR                        0x20000

/**
 * @def NULL_CHECK_REMOVER
 * @ingroup Codetools
 * @brief Remove checks for NULL
 *
 * Analyse instructions to remove checks for NULL that are not needed.
 */
#define NULL_CHECK_REMOVER                      0x40000

/**
 * @def POST_DOMINATOR_COMPUTER
 * @ingroup Codetools
 * @brief Compute a post-dominance tree
 *
 * Analyse a method, computing a post-dominance tree for relationships between instructions.
 */
#define POST_DOMINATOR_COMPUTER                 0x80000

/**
 * @def PRE_DOMINATOR_COMPUTER
 * @ingroup Codetools
 * @brief Compute a pre-dominance tree
 *
 * Analyse a method, computing a pre-dominance tree for relationships between instructions.
 */
#define PRE_DOMINATOR_COMPUTER                  0x100000

/**
 * @def LOOP_UNROLLING
 * @ingroup Codetools
 * @brief Unroll loops
 *
 * Analyse a method and unroll profitable loops.
 */
#define LOOP_UNROLLING                          0x200000

/**
 * @def LOOP_IDENTIFICATION
 * @ingroup Codetools
 * @brief Identify loops
 *
 * Analyse a method, identifying loops within it.
 */
#define LOOP_IDENTIFICATION                     0x400000

/**
 * @def LOOP_INVARIANT_CODE_HOISTING
 * @ingroup Codetools
 * @brief Move invariant code out of loops
 *
 * Identify code that is invariant across iterations of a loop and hoist it above the loop.
 */
#define LOOP_INVARIANT_CODE_HOISTING            0x800000

/**
 * @def LOOP_INVARIANT_VARIABLES_ELIMINATION
 * @ingroup Codetools
 * @brief Eliminate invariant variables from loops
 *
 * Identify variables that are invariant across loop iterations and remove them.
 */
#define LOOP_INVARIANT_VARIABLES_ELIMINATION    0x1000000

/**
 * @def LOOP_UNSWITCHING
 * @ingroup Codetools
 * @brief Unswitch loops
 *
 * Move conditional code outside of loops.
 */
#define LOOP_UNSWITCHING                        0x2000000

/**
 * @def LOOP_TO_WHILE
 * @ingroup Codetools
 * @brief Transform do-while loops
 *
 * Transform loops in do-while form so that the condition is at the start of the loop, representing a while loop.
 */
#define LOOP_TO_WHILE		             	0x4000000

/**
 * @def LOOP_PEELING
 * @ingroup Codetools
 * @brief Peel iterations from loops
 *
 * Peel the first @c n iterations from the start of a loop so that they are executed sequentially before the loop.
 */
#define LOOP_PEELING                            0x8000000

/**
 * @def BOUNDS_CHECK_REMOVAL
 * @ingroup Codetools
 * @brief Remove bounds checks
 *
 * Remove checks on array bounds.
 */
#define BOUNDS_CHECK_REMOVAL                    0x10000000

/**
 * @def STRENGTH_REDUCTION
 * @ingroup Codetools
 * @brief Perform strength reduction
 *
 * Replace expensive algebraic calculations with simpler versions.
 */
#define STRENGTH_REDUCTION                      0x20000000

/**
 * @def ALGEBRAIC_SIMPLIFICATION
 * @ingroup Codetools
 * @brief Simplify algebraic expressions
 *
 * Replace algebraic expressions, where possible, with simpler versions.
 */
#define ALGEBRAIC_SIMPLIFICATION                0x40000000

/**
 * @def COPY_PROPAGATOR
 * \ingroup Codetools
 * @brief Copy propagation code transformation
 *
 * It performs the copy propagation code transformation
 */
#define COPY_PROPAGATOR                         0x80000000

/**
 * @def METHOD_INLINER
 * @ingroup Codetools
 * @brief Inline methods
 *
 * Inline code from called methods into the current method, when profitable.
 */
#define METHOD_INLINER                          0x100000000LLU

/**
 * @def VARIABLES_RENAMING
 * @ingroup Codetools
 * @brief Rename variables
 *
 * Rename variables so that low IDs are used, to optimize the performance of the compiler.
 */
#define VARIABLES_RENAMING                      0x200000000LLU

/**
 * @def VARIABLES_LIVE_RANGE_SPLITTING
 * @ingroup Codetools
 * @brief Perform live-range splitting
 *
 * Split variables that have long live ranges.
 */
#define VARIABLES_LIVE_RANGE_SPLITTING          0x400000000LLU

/**
 * @def INDIRECT_CALL_ELIMINATION
 * @ingroup Codetools
 * @brief Eliminate indirect calls
 *
 * Replace indirect calls with direct calls if there is only a single possible target method.
 */
#define INDIRECT_CALL_ELIMINATION               0x800000000LLU

/**
 * @def NATIVE_METHODS_ELIMINATION
 * @ingroup Codetools
 * @brief Eliminate native methods
 *
 * Remove native methods to known functions and replace them with IR instructions (e.g., calls to @c sqrt become <code> IRSQRT </code>).
 */
#define NATIVE_METHODS_ELIMINATION              0x1000000000LLU

/**
 * @def DUMPCODE
 * @ingroup Codetools
 * @brief Dump a method's code
 *
 * Write out the code of a method to a file.
 */
#define DUMPCODE                                0x2000000000LLU

/**
 * @def SSA_CONVERTER
 * \ingroup Codetools
 * @brief Convert the method code in SSA form
 *
 * It converts the method code in Static Single Assignment form, in which the
 * value of each variable is defined in only one point of the method.
 */
#define SSA_CONVERTER                           0x4000000000LLU

/**
 * @def SSA_BACK_CONVERTER
 * \ingroup Codetools
 * @brief Convert the method code from SSA form to normal form
 *
 * It converts the method code from Static Single Assignment form back to
 * normal executable form.
 */
#define SSA_BACK_CONVERTER                      0x8000000000LLU

/**
 * @def CALL_DISTANCE_COMPUTER
 * @ingroup Codetools
 * @brief Compute call distances
 *
 * Compute the distance of each call instruction from the first in the method.
 */
#define CALL_DISTANCE_COMPUTER                  0x10000000000LLU

/**
 * @def CONVERSION_MERGING
 * @ingroup Codetools
 * @brief Merge conversion instructions
 *
 * Merge sequences of @c IRCONV instructions together to make shorter sequences, eliminating unnecessary instructions.
 */
#define CONVERSION_MERGING                      0x20000000000LLU

/**
 * @def ESCAPES_ELIMINATION
 * \ingroup Codetools
 * @brief Eliminates accesses to local variables through loads and stores
 *
 * This codetool tries to eliminate accesses to method local variables through loads and stores.
 * The transformed method uses them by direct accesses instead.
 */
#define ESCAPES_ELIMINATION                     0x40000000000LLU

/**
 * @def THREADS_EXTRACTION
 * \ingroup Codetools
 * @brief HELIX codetool
 *
 * These codetools extract coarse grain threads from sequential code.
 */
#define THREADS_EXTRACTION                      0x80000000000LLU

/**
 * @def CONSTANT_FOLDING
 * \ingroup Codetools
 * @brief Constant folding codetool
 *
 * These codetools fold constants
 */
#define CONSTANT_FOLDING                        0x100000000000LLU

/**
 * @def DDG_PROFILE
 * \ingroup Codetools
 * @brief Data dependence graph profile
 *
 * Profile the DDG at run time.
 */
#define DDG_PROFILE                         	0x200000000000LLU

/**
 * @def REACHING_INSTRUCTIONS_ANALYZER
 * \ingroup Codetools
 * @brief Reaching instruction analysis
 *
 * It computes the reachability of each instruction of a method.
 */
#define REACHING_INSTRUCTIONS_ANALYZER          0x400000000000LLU

/**
 * @def LOOP_PROFILE
 * \ingroup Codetools
 * @brief Profile loop execution
 *
 * Profile the execution of loops.
 */
#define LOOP_PROFILE                         	0x800000000000LLU

/**
 * @def CUSTOM
 * \ingroup Codetools
 * @brief Custom codetool
 *
 * This type of codetool is for extensions that do not belong to any other type.
 */
#define CUSTOM                                  0x1000000000000LLU

#endif
