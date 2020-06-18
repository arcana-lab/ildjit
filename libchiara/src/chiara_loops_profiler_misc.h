/*
 * Copyright (C) 2010 - 2012  Campanoni Simone
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
#ifndef CHIARA_LOOPS_PROFILER_MISC_H
#define CHIARA_LOOPS_PROFILER_MISC_H

#include <xanlib.h>

typedef struct {
    void *start;
    void *end;
} memory_t;

XanList * get_profiled_loops (XanList *loops, XanHashTable *loopsNames);

void profileLoop (ir_optimizer_t *irOptimizer, loop_profile_t *profile, void *startLoopFunction, ir_item_t *startLoopFunctionReturnItem, XanList *startLoopFunctionParameters, void *endLoopFunction, ir_item_t *endLoopFunctionReturnItem, XanList *endLoopFunctionParameters, void *newIterationFunction, ir_item_t *newIterationFunctionReturnItem, XanList *newIterationFunctionParameters, XanList *loopSuccessors, XanList *loopPredecessors, void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst));

void profileLoopByUsingSymbols (ir_optimizer_t *irOptimizer, loop_profile_t *profile, ir_symbol_t *startLoopFunction, ir_item_t *startLoopFunctionReturnItem, XanList *startLoopFunctionParameters, ir_symbol_t *endLoopFunction, ir_item_t *endLoopFunctionReturnItem, XanList *endLoopFunctionParameters, ir_symbol_t *newIterationFunction, ir_item_t *newIterationFunctionReturnItem, XanList *newIterationFunctionParameters, XanList *successors, XanList *predecessors, XanList *addedInstructions);

XanList * instrument_loops (	ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, void (*newLoop)(void *loopName), void (*endLoop)(void), void (*newIteration)(void), void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst), IR_ITEM_VALUE (*fetchNewLoopParameter)(loop_profile_t *p));

#endif
