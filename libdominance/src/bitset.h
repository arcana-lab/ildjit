/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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
#ifndef BISET_H
#define BISET_H

#include <stddef.h>

/*! \typedef typedef struct bitset
 *  \brief Structure for bitset
 *
 * data holds the bitset itsef and length contains the number of bits
 * of the current bitset
 */
typedef struct {
    size_t *data;
    size_t length;
} bitset;

/*! \fn bitset *bitset_alloc(size_t length)
 * \brief allocate a bitset structure in memory and return a pointer
 */
inline bitset *bitset_alloc (size_t length);

//void bitset_print_i(bitset *set);

/*! \fn void bitset_clear_all(bitset *set)
 * \brief set to 0 all the bits in the bitset
 */
inline void bitset_clear_all (bitset *set);

/*! \fn void bitset_set_all(bitset *set)
 * \brief set to 1 all the bits in the bitset
 */
inline void bitset_set_all (bitset *set);

/*! \fn void bitset_free(bitset *set)
 * \brief release the memory used for the bitset
 */
inline void bitset_free (bitset *set);

/*! \fn void bitset_intersect(bitset *dest, bitset *src)
 * \brief compute the intersection between dest and src and assign it to dest
 */
inline void bitset_intersect (bitset *dest, bitset *src);
inline void bitset_union (bitset *dest, bitset *src);

/*! \fn int bitset_equal(bitset *bs1, bitset *bs2)
 * \brief test if bs1 is equal to bs2
 */
inline int bitset_equal (bitset *bs1, bitset *bs2);

/*! \fn void bitset_clone(bitset *dest, bitset *src)
 * \brief copy src to dest
 */
inline void bitset_clone (bitset *dest, bitset *src);

/*! \fn void bitset_clear_bit(bitset *set, size_t pos)
 * \brief set the pos-st bit to 0
 */
inline void bitset_clear_bit (bitset *set, size_t pos);

/*! \fn void bitset_set_bit(bitset *set, size_t pos)
 * \brief set the pos-st bit to 1
 */
inline void bitset_set_bit (bitset *set, size_t pos);

/*! \fn int bitset_test_bit(bitset *set, size_t pos)
 * \brief return the value of the pos-st bit
 */
inline int bitset_test_bit (bitset *set, size_t pos);

/*! \fn bitset_print(bitset *set, int cr)
 * \brief print the bitset to the standard output. Set cr if you want a newline
 */
inline void bitset_print (bitset *set, int cr);

#endif
