/*
 * Copyright (C) 2008 - 2011 Campanoni Simone
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
#ifndef OPTIMIZATION_LEVELS_H
#define OPTIMIZATION_LEVELS_H

#include <ir_method.h>
#include <xanlib.h>

// My headers
#include <ir_optimizer.h>
// End

void initialize_optimizationLevels_plugins (ir_optimizer_t *lib);
JITUINT32 optimizeMethod_AOT_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint);
JITUINT32 optimizeMethod_PostAOT_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
JITUINT32 optimizeMethod_atLevel (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint);

#endif
