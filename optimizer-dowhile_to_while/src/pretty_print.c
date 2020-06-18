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
 * @file pretty_print.c
 * @brief Simple functions to print a prettier textual output
 * @author Mastrandrea Daniele
 * @date 2010
**/

#include <assert.h>		/* assert()		*/
#include <stdio.h>		/* vfprintf() & co.	*/
#include <stdarg.h>		/* va_args, va_start	*/
#include <stdlib.h>		/* abs()		*/
#include "pretty_print.h"	/* [...]		*/

void pretty_init(PP_t * printer) {
	assert(NULL != printer);
	printer->indentation_level = 0;
	printer->marked = MARKED;
}

void pretty_indent(PP_t * printer) {
	assert(printer->indentation_level >= 0);
	(printer->indentation_level)++;
}

void pretty_unindent(PP_t * printer) {
	assert(printer->indentation_level > 0);
	(printer->indentation_level)--;
}

void pretty_mark(PP_t * printer) {
	assert(UNMARKED == printer->marked);
	printer->marked = 1;
}

void pretty_unmark(PP_t * printer) {
	assert(MARKED == printer->marked);
	printer->marked = 0;
}

int pretty_fprintf(PP_t * printer, FILE * stream, const char * fmt, ...) {
	int i, ret;
	va_list args;

	va_start(args, fmt);
	ret = 0;

	for (i = 1; i < abs(printer->indentation_level); i++)
		ret += fprintf(stream, STANDARD_INDENTATION);

	if (MARKED == printer->marked) {
		switch (printer->indentation_level) {
			case 0:
				break;
			case 1:
				ret += fprintf(stream, "- ");
				break;
			case 2:
				ret += fprintf(stream, ". ");
				break;
			case 3:
				ret += fprintf(stream, "* ");
				break;
			case 4:
				ret += fprintf(stream, "+ ");
				break;
			case 5:
				ret += fprintf(stream, "> ");
				break;
			default:
				ret += fprintf(stream, "_ ");
				break;
		}
	} else
		ret += fprintf(stream, "  ");

	ret += vfprintf(stream, fmt, args);
	return ret;
}