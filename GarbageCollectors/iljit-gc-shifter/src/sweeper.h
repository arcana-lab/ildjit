/*
 * Copyright (C) 2007  Campanoni Simone
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
#ifndef SWEEPER_H
#define SWEEPER_H

// My headers
#include <memory_manager.h>
#include <iljit_gc_shifter.h>
#include <iljit-utils.h>
// End

JITUINT32 sweep_the_heap_from_bottom_to_top (t_memory *memory, XanList *rootSets, XanList *objectsLived, t_gc_behavior *gc);
JITUINT32 sweep_the_heap_from_top_to_bottom (t_memory *memory, XanList *rootSets, XanList *objectsLived, t_gc_behavior *gc);
JITUINT32 sweep_the_objects_references (t_memory *memory, JITNUINT *objectsReferenceMark, JITUINT32 objectsReferenceLength);

#endif
