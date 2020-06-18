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
#ifndef INLINE_HEURISTICS_H
#define INLINE_HEURISTICS_H

#include <ir_method.h>
#include <xanlib.h>

XanList * 	fetchMethodsToConsider (void);
void 		remove_calls (ir_method_t *startMethod, XanList *calls);
XanList * 	fetchCallInstructions (ir_method_t *method, JITUINT32 max_insns_without_loops, JITUINT32 max_insns_with_loops, JITUINT32 max_insns, XanHashTable *methodsToAvoid);
JITBOOLEAN	inlinedBlockedForMaxInsnsWithoutLoopsInputConstraints;
JITBOOLEAN	inlinedBlockedForMaxInsnsWithLoopsInputConstraints;
JITBOOLEAN	inlinedBlockedForMaxInsnsWithLoopsInputConstraints;
JITBOOLEAN	inlinedBlockedForMaxCallerInsnsInputConstraints;
JITUINT32	minInsnsWithoutLoopsInputConstraintsToUnblockInline;
JITUINT32	minInsnsWithLoopsInputConstraintsToUnblockInline;
JITUINT32	minCallerInsnsToUnblockInline;
JITBOOLEAN 	hasLoop (ir_method_t *m);

#endif
