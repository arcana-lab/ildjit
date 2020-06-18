/*
 * Copyright (C) 2010  Mastrandrea Daniele
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
 * @file pretty_print.h
 * @brief Simple functions to print a prettier textual output
 * @author Mastrandrea Daniele
 * @date 2010
 **/

#ifndef PRETTY_PRINT_H
#define PRETTY_PRINT_H

#include <stdio.h>              /* vfprintf() & co.	*/

#define STANDARD_INDENTATION "    "

/**
 * @defgroup PrettyPrint
 * Simple functions to print a prettier textual output
 **/

/**
 * @ingroup PrettyPrint
 * @brief Increase the indentation level of output.
 **/
void pretty_indent (int * indentation_level);

/**
 * @ingroup PrettyPrint
 * @brief Decrease the indentation level of output.
 **/
void pretty_unindent (int * indentation_level);

/**
 * @ingroup PrettyPrint
 * @brief Force a mark to be placed before the output with indentation level >= 1.
 **/
void pretty_mark (int * indentation_level);

/**
 * @ingroup PrettyPrint
 * @brief Avoid tha mark to be placed.
 **/
void pretty_unmark (int * indentation_level);

/**
 * @ingroup PrettyPrint
 * @brief Print to standard output according to the indentation level.
 **/
int pretty_fprintf (int * indentation_level, FILE * stream, const char * fmt, ...);

#endif /* PRETTY_PRINT_H */