/*
 * Copyright (C) 2008  Campanoni Simone
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
#ifndef OPTIMIZATIONS_UTILITIES_H
#define OPTIMIZATIONS_UTILITIES_H

#include <ir_method.h>
#include <jitsystem.h>
#include <xanlib.h>

// My headers
#include <ir_optimizer.h>
#include <ir_optimization_interface.h>
#include <ir_optimization_levels_interface.h>
// End

typedef struct {
    void                                    *handle;
    ir_optimization_interface_t             *plugin;
    JITBOOLEAN				enabled;
} ir_optimization_t;

typedef struct {
    void                                    *handle;
    ir_optimization_levels_interface_t      *plugin;
} ir_optimization_levels_t;

void internal_callMethodOptimization (ir_optimizer_t *lib, ir_method_t *method, JITUINT64 optimizationKind, JITBOOLEAN checkWhetherIsEnabled);
void internal_optimizeMethod_simple_iteration (ir_optimizer_t *lib, ir_method_t *method);
JITBOOLEAN internal_isOptimizationEnabled (ir_optimizer_t *lib, JITUINT64 optimizationKind, JITBOOLEAN anyOneEnabled);
JITBOOLEAN internal_isOptimizationInstalled (ir_optimizer_t *lib, JITUINT64 optimizationKind);

#endif
