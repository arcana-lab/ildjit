/*
 * Copyright (C) 2009  Campanoni Simone
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
#ifndef DOMINANCE_H
#define DOMINANCE_H

#include <xanlib.h>

/**
 * @brief auxiliary structure used by preDominance algorithm
 *
 * In this structure for each node will be saved a pointer to node in the preDominance tree,
 * the level in the tree and flag to avoid linking with deadcode
 */
typedef struct {                        //	auxiliary structure that contains for each statements:
    XanNode *node;                  //  one pointer to node into preDominanceTree
    int level;                      //  and deep level (root level is zero)
} nodeDeep;

XanNode* calculateDominanceTree(void** elems, int numElems, int headNodePos, void* (*getPredecessors)(void* elem, void* pred), void* (*getSuccessors)(void* elem, void* succ));
JITBOOLEAN isADominator(void *elem, void *dominated, XanNode *tree);
XanList *getElemsDominatingElem(void *dominated, XanNode *tree);

#endif
