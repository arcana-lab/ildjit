/*
 * Copyright (C) 2006  Campanoni Simone
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


#ifndef CONTROL_DEPENDENCE_GRAPH_H
#define CONTROL_DEPENDENCE_GRAPH_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#endif

/*
   typedef struct {
        size_t *bits;
        size_t nbits;
   } bitset;

   bitset *bitset_init(size_t nbits);
   void bitset_reset(bitset *set);
   void bitset_set(bitset *set);
   void bitset_free(bitset *set);
   void bitset_intersect(bitset *dest, bitset *src);
   int bitset_equal(bitset *bs1, bitset *bs2);
   void bitset_copy(bitset *dest, bitset *src);
   void bitset_clear_bit(bitset *set, size_t pos);
   void bitset_set_bit(bitset *set, size_t pos);
   int bitset_test_bit(bitset *set, size_t pos);
   void bitset_print(bitset *set);
 */
