/*
 * Copyright (C) 2008 - 2012 Timothy M. Jones, Simone Campanoni
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
 * @file optimizer_ddg.h
 * @brief Plugin for Data Dependency Graph.
 */
#ifndef OPTIMIZER_DDG_H
#define OPTIMIZER_DDG_H

#include <iljit-utils.h>
#include <ir_optimization_interface.h>

#ifdef PRINTDEBUG
extern JITBOOLEAN enablePDebug;
#define PDEBUG(fmt, args...) if (enablePDebug) fprintf(stderr, "OPT_DDG: " fmt, ## args)
#define PDEBUGB(fmt, args...) if (enablePDebug) fprintf(stderr, fmt, ## args)
#define PDEBUGNL(fmt, args...) if (enablePDebug) fprintf(stderr, "OPT_DDG: \nOPT_DDG: " fmt, ## args)
#define SET_DEBUG_ENABLE(val) enablePDebug = val
#define GET_DEBUG_ENABLE(var) var = enablePDebug
#else
#define enablePDebug JITFALSE
#define PDEBUG(fmt, args...)
#define PDEBUGB(fmt, args...)
#define PDEBUGNL(fmt, args...)
#define SET_DEBUG_ENABLE(val)
#define GET_DEBUG_ENABLE(var)
#endif

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    ir_instruction_t *inst;
    JITNUINT escapeVariableID;
} escapeInfo_t;

/**
 * Add a new possible dependence for an instruction.  The 'fromInst'
 * corresponds to the 'depending' instruction in the function
 * IRMETHOD_addInstructionDataDependence.
 **/
void DDG_addPossibleDependence(ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst, JITUINT32 depType);

/**
 * Compare two instructions by ID.
 */
int compareInstructionIDs(const void *i1, const void *i2);

/**
 * Global variables for this pass.
 */
extern ir_lib_t *irLib;
extern ir_optimizer_t *irOptimizer;

extern JITINT32 provideMemoryAllocators;

void printDependences(ir_method_t *method, FILE *file);

#endif
