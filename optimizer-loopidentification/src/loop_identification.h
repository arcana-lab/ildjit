/*
 * Copyright (C) 2008 Simone Campanoni, Castiglioni William
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
#ifndef LOOP_IDENTIFICATION_H
#define LOOP_IDENTIFICATION_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif
#define DIM_BUF	2048

/*
 * @brief auxiliary structure used by preDominance algorithm
 *
 * In this structure for each node will be saved a pointer to node in the preDominance tree and
 * the level in the tree and flag to avoid linking with deadcode
 */
struct Back_Edge {
    JITNUINT Source;        // da dove parte
    JITNUINT Dest;          // header del loop,dove arriva
};
typedef struct  Back_Edge BackEdge;

#endif
