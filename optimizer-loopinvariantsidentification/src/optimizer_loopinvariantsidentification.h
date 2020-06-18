/*
 * Copyright (C) 2008  Borgio Simone, Bosisio Davide, Alessandro Licata Caruso
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

#ifndef OPTIMIZER_LOOPINVARIANTSIDENTIFICATION_H

#define OPTIMIZER_LOOPINVARIANTSIDENTIFICATION_H

#define _GET_BIT_AT(bitmap, element, block_size)  \
	((					    \
		 (					   \
			 (bitmap)[(element) / (block_size)]	   \
			 &				       \
			 (0x1 << (element) % (block_size))	   \
		 )					   \
		 != 0 ? 1 : 0				   \
		 ))

#ifdef PRINTDEBUG

#define PDBG_P(fmt, args ...)				     \
	{							\
		fprintf(stderr, "LOOP_INVARIANTS_IDENTIFICATION: ");  \
		fprintf(stderr, fmt, ## args);			      \
	}

#define PDBG(fmt, args ...) fprintf(stderr, fmt, ## args);

#define _PRINT_BITMAP(bitmap, bitmap_size, block_size, tmp1)  \
	{						      \
		PDBG("{ ");					    \
		for ((tmp1) = 0; (tmp1) < (bitmap_size); (tmp1)++) {  \
			if (_GET_BIT_AT((bitmap), (tmp1), (block_size))) {  \
				PDBG("%d ", (tmp1)); } }			  \
		PDBG("}\n");					    \
	}

#else

#define PDBG_P(fmt, args ...)
#define PDBG(fmt, args ...)
#define _PRINT_BITMAP(bitmap, bitmap_size, block_size, tmp1)

#endif

#endif
