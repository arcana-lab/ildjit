/*
 * Copyright (C) 2009-2010  Mastrandrea Daniele
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
 * @file bits.c
 * @brief Bit(s) manipulation functions
 * @author Mastrandrea Daniele
 * @date 2009-2010
**/

#include <assert.h>	/* assert()		*/
#include <stdlib.h>	/* malloc()		*/
#include <stddef.h>	/* NULL			*/
#include <limits.h>	/* CHAR_BIT		*/
#include <string.h>	/* strdup()		*/
#include <math.h>	/* pow()		*/
#include <jitsystem.h>	/* JITUINT32, JITNUINT	*/
#include "bits.h"	/* [...]		*/

#define allocFunction malloc

const char * blk_to_string(JITNUINT bits) {
	return blks_to_string(&bits, sizeof(bits));
}

const char * blks_to_string(JITNUINT * bits, JITUINT32 n_instructions) {
	int i, j;
	int current_bit = 0;
	const int n_bits = sizeof(*bits)*CHAR_BIT;
	const int n_blocks = ceil((double)n_instructions/(double)n_bits);
	char * bit_string = allocFunction(n_blocks*n_bits+1);
	assert(n_blocks >= 1);
	for (i = n_blocks-1; i >= 0; i--) {
		for (j = n_bits-1; j >= 0; j--) {
			*(bit_string+current_bit++) = 0 == ((bits[i])>>j) % 2 ? '0' : '1';
		}
	}
	bit_string[n_blocks*n_bits] = '\0';
	return strdup(bit_string);
}

JITUINT32 find_first_bit(JITNUINT * bits, JITUINT32 n_instructions) {
	int i, j;
	const int n_bits = sizeof(*bits)*CHAR_BIT;
	const int n_blocks = ceil((double)n_instructions/(double)n_bits);
	const JITNUINT mask = (JITNUINT) 1;
	assert(n_blocks >= 1);
	for (i = 0; i < n_blocks; i++) {
		if (0 == bits[i])
			continue;
		for (j = 0; j < n_bits; j++) {
			if (mask == (bits[i]>>j & mask))
				return i*n_bits+j;
		}
	}
	return (JITUINT32) (JITNUINT)NULL;
}

JITUINT32 find_last_bit(JITNUINT * bits, JITUINT32 n_instructions) {
	int i, j;
	const int n_bits = sizeof(*bits)*CHAR_BIT;
	const int n_blocks = ceil((double)n_instructions/(double)n_bits);
	const JITNUINT mask = (JITNUINT) pow(2, n_bits-1);
	assert(n_blocks >= 1);   
	for (i = n_blocks-1; i >= 0; i--) {
		if (0 == bits[i])
			continue;
		for (j = 0; j < n_bits; j++) {
			if (mask == (bits[i]<<j & mask))
				return i*n_bits+n_bits-j-1;
		}
	}
	return (JITUINT32) (JITNUINT) NULL;
}

JITUINT32 count_bits(JITNUINT * bits, JITUINT32 n_instructions) {
	int i, j;
	JITUINT32 count = 0;
	const int n_bits = sizeof(*bits)*CHAR_BIT;
	const int n_blocks = ceil((double)n_instructions/(double)n_bits);
	const JITNUINT mask = (JITNUINT) 1;
	assert(n_blocks >= 1);
	for (i = 0; i < n_blocks; i++) {
		if (0 == bits[i])
			continue;
		for (j = 0; j < n_bits; j++) {
			if (mask == (bits[i]>>j & mask))
				count++;
		}
	}
	return count;
}
