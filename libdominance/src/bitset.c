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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "bitset.h"

/* useful constants	*/

/* number of bits in a word	*/
#define BITS_IN_A_WORD ( 8 * sizeof(size_t) )

/* mask with ths pos-st bit set. Ex: pos = 9 => 0000000001000000000000000000000	*/
#define MASK(pos) ( ((size_t) 1) << ((pos) % BITS_IN_A_WORD) )

/* the number of the word on which the mask should be applied	*/
#define WORD_NUMBER(set, pos) ( (set)->data[(pos) / BITS_IN_A_WORD] )

/* number of words necessary to hold a bitset with length length	*/
#define WORDS_NECESSARY(length)	\
	( ((length) + (BITS_IN_A_WORD - 1)) / BITS_IN_A_WORD )

inline bitset *bitset_alloc (size_t length) {
    bitset *set;

    set = malloc(sizeof(*set));
    assert(set);

    set->data = calloc(WORDS_NECESSARY(length), sizeof(*set->data));
    set->length = length;

    assert(set->data);

    return set;
}

inline void bitset_intersect (bitset *dest, bitset *src) {
    int i;

    for (i = 0; i < WORDS_NECESSARY(src->length); ++i) {
        dest->data[i] &= src->data[i];
    }
}

inline void bitset_union (bitset *dest, bitset *src) {
    int i;

    for (i = 0; i < WORDS_NECESSARY(src->length); ++i) {
        dest->data[i] |= src->data[i];
    }
}

inline int bitset_equal (bitset *bs1, bitset *bs2) {
    int i;

    if (bs1->length != bs2->length) {
        return 0;
    }
    for (i = 0; i < WORDS_NECESSARY(bs1->length); i++) {
        if (bs1->data[i] != bs2->data[i]) {
            return 0;
        }
    }
    return 1;
}

inline void bitset_clone (bitset *dest, bitset *src) {
    //src e dest devono avere la stessa dimensione
    memcpy(dest->data, src->data, WORDS_NECESSARY(src->length) * sizeof(*src->data));
}

inline void bitset_print (bitset *set, int cr) {
    int i;

    for (i = 0; i < set->length; i++) {
        fprintf(stdout, "%d", ((WORD_NUMBER(set, i) & MASK(i)) != 0));
    }
    if (cr) {
        fprintf(stdout, "\n");
    }
}

/*
 * OBSOLETO
 * void bitset_print_i(bitset *set)
   {
        int i;
        for(i=0; i < (WORDS_NECESSARY(set->length) * sizeof(*set->data))*8; i++){
                fprintf(stdout, "%d", ((WORD_NUMBER(set, i) & MASK(i))!=0));
        }
        fprintf(stdout, "\n");
   }*/

inline void bitset_clear_all (bitset *set) {
    //WORDS_NECESSARY(set->length) * sizeof(*set->data) = n° di Byte occupati dal bitset
    memset(set->data, 0, WORDS_NECESSARY(set->length) * sizeof(*set->data));
}

inline void bitset_set_all (bitset *set) {
    //memset(void *s, int c, size_t n);
    //0xFF = 11111111
    memset(set->data, 0xFF, WORDS_NECESSARY(set->length) * sizeof(*set->data));
}

inline void bitset_free (bitset *set) {
    free(set->data);
    free(set);
}

inline void bitset_clear_bit (bitset *set, size_t pos) {
    if (pos >= set->length) {
        printf("Errore");
        abort();
    }

    WORD_NUMBER(set, pos) &= ~MASK(pos);
}

inline void bitset_set_bit (bitset *set, size_t pos) {
    if (pos >= set->length) {
        printf("Errore");
        abort();
    }

    WORD_NUMBER(set, pos) |= MASK(pos);
}

inline int bitset_test_bit (bitset *set, size_t pos) {
    if ((pos >= set->length) ||
            (pos < 0)           ) {
        printf("Errore");
        abort();
    }

    return (WORD_NUMBER(set, pos) & MASK(pos)) != 0;
}
