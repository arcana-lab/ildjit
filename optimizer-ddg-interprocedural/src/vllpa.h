/*
 * Copyright (C) 2011 Timothy M. Jones
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
 * @file vllpa.h
 * @brief Plugin for Data Dependency Graph Generation.
 */

#ifndef VLLPA_H
#define VLLPA_H

#include <iljit-utils.h>
#include <ir_optimization_interface.h>


/**
 * Entry point to the VLLPA algorithm.  Assumes that the given method is
 * the main method or method to use as the start point for the call graph.
 */
void doVllpa(ir_method_t *method);

#endif
