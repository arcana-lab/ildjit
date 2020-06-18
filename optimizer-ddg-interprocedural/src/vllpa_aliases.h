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

#ifndef VLLPA_ALIASES_H
#define VLLPA_ALIASES_H

/**
 * This file contains types and prototypes for the alias detection part
 * of the interprocedural pointer analysis.
 */

#include <xanlib.h>

#include "smalllib.h"
#include "vllpa_macros.h"


/**
 * Compute the memory dependences between instructions within each method
 * within an SCC.
 */
void computeMemoryDependencesInSCC(XanList *scc, SmallHashTable *methodsInfo, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap);

/**
 * Print out the number of memory data dependences identified.
 */
void printMemoryDataDependenceStats(void);


#endif  /* VLLPA_ALIASES_H */
