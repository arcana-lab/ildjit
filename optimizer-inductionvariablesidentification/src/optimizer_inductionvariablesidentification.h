/*
 * Copyright (C) 2008 - 2010 Simone Campanoni, Borgio Simone, Bosisio Davide, Licata Caruso Alessandro
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
#ifndef OPTIMIZER_INDUCTIONVARIABLESIDENTIFICATION_H
#define OPTIMIZER_INDUCTIONVARIABLESIDENTIFICATION_H

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

/**
 * Debug messages printing utility.
 * Prints on \c stderr a debug message with a convenient prefix.
 */
#define PDBG_P(fmt, args ...)			   \
	{					      \
		fprintf(stderr, "INDUCTION_VARIABLES_IDENTIFICATION: ");  \
		fprintf(stderr, fmt, ## args);		    \
	}
/**
 * Debug messages printing utility.
 * Prints on \c stderr a debug message.
 */
#define PDBG(fmt, args ...) fprintf(stderr, fmt, ## args);

#define _PRINT_BITMAP(bitmap, bitmap_size, block_size, tmp1)  \
	{						      \
		PDBG("{ ");					    \
		for ((tmp1) = 0; (tmp1) < (bitmap_size); (tmp1)++) {  \
			if (_GET_BIT_AT((bitmap), (tmp1), (block_size))) {  \
				PDBG("%d ", (tmp1)); } }			  \
		PDBG("}\n");					    \
	}
/**
 * Prints a detailed view of the induction variables of the loop.
 * @param induction_bmp a pointer to the first block of the induction
 *                      variables bitmap.
 * @param induction_ht  a pointer to the induction variable hash table
 *                      related to <tt>induction_bmp</tt>.
 * @param bmp_elements  the length of <tt>induction_bmp</tt>.
 * @param block_size    the size (in bit) of a bitmap block.
 * @param tmp_ui32      a \c JITUINT32 dummy variable.
 * @param tmp_ivt_ptr   an \c induction_variable_t* dummy variable.
 */
#define _PRINT_INDUCTION_VARS(induction_bmp, induction_ht, bmp_elements, \
				  block_size, tmp_ui32, tmp_ivt_ptr)	     \
	{ \
		for ((tmp_ui32) = 0; (tmp_ui32) < (bmp_elements); (tmp_ui32)++)	\
		{ \
			if (_GET_BIT_AT((induction_bmp), (tmp_ui32), (block_size)))  \
			{ \
				PDBG_P("\tVar%d:\n", (tmp_ui32)); \
				(tmp_ivt_ptr) = (induction_ht)->lookup((induction_ht), \
								       &(tmp_ui32)); \
				if ((tmp_ivt_ptr) != NULL)  \
				{ \
					PDBG_P("\t\tID = %d\n", (tmp_ivt_ptr)->ID); \
					PDBG_P("\t\tVar%d = ", (tmp_ui32)); \
					if ((tmp_ivt_ptr)->a.type == IROFFSET) \
					{ \
						PDBG("Var%lld + ", (tmp_ivt_ptr)->a.value); \
					} \
					else if ((tmp_ivt_ptr)->a.type == (JITUINT32) -IROFFSET) \
					{ \
						PDBG("-Var%lld + ", (tmp_ivt_ptr)->a.value); \
					} \
					else if ((tmp_ivt_ptr)->a.type == IRINT64) \
					{ \
						PDBG("%lld + ", (tmp_ivt_ptr)->a.value); \
					} \
					if ((tmp_ivt_ptr)->b.type == IROFFSET) \
					{ \
						PDBG("Var%lld*", (tmp_ivt_ptr)->b.value); \
					} \
					else if ((tmp_ivt_ptr)->b.type == (JITUINT32) -IROFFSET) \
					{ \
						PDBG("-Var%lld*", (tmp_ivt_ptr)->b.value); \
					} \
					else if ((tmp_ivt_ptr)->b.type == IRINT64) \
					{ \
						PDBG("%lld*", (tmp_ivt_ptr)->b.value); \
					} \
					PDBG("Var%d.\n", (tmp_ivt_ptr)->i); \
				} \
				else{  \
					PDBG_P("\t\tUnavailable informations.\n"); } \
			} \
		} \
	}

#else

#define PDBG_P(fmt, args ...)
#define PDBG(fmt, args ...)
#define _PRINT_BITMAP(bitmap, bitmap_size, block_size, tmp1)
#define _PRINT_INDUCTION_VARS(induction_bmp, induction_ht, bmp_elements,  block_size, tmp_ui32, tmp_ivt_ptr)

#endif

#endif
