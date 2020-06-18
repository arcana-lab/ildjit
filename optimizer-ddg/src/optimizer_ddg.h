/*
 * Copyright (C) 2008  Ceriani Marco
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

/**
 *\mainpage Plugin Data Dependency Graph
 *
 * \author Michele Tartara
 * \version 0.0.1
 */

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

typedef struct {
	t_ir_instruction *inst;
	JITNUINT escapeVariableID;
} escapeInfo_t;

#endif
