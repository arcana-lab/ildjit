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
 * @file bits.h
 * @brief Bit(s) manipulation functions
 * @author Mastrandrea Daniele
 * @date 2009-2010
**/

#ifndef BITS_H
#define BITS_H

#include <jitsystem.h>	/* JITUINT32, JITNUINT	*/

/**
 * @defgroup Bits
 * Bit(s) manipulation functions
**/

/**
 * @ingroup Bits
 * @brief Returns a string representing a number in the binary encoding.
**/
const char * blk_to_string(JITNUINT bits);

/**
 * @ingroup Bits
 * @brief Returns a string representing a number in the binary encoding.
**/
const char * blks_to_string(JITNUINT * bits, JITUINT32 n_instructions);

/**
 * @ingroup Bits
 * @brief Returns the index of the most significant bit set to one.
**/
JITUINT32 find_first_bit(JITNUINT * bits, JITUINT32 n_instructions);

/**
 * @ingroup Bits
 * @brief Returns the index of the least significant bit set to one.
**/
JITUINT32 find_last_bit(JITNUINT * bits, JITUINT32 n_instructions);

/**
 * @ingroup Bits
 * @brief Returns the number of bits set to one.
**/
JITUINT32 count_bits(JITNUINT * bits, JITUINT32 n_instructions);

#endif /* BITS_H */