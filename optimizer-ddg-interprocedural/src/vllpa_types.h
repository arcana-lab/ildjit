/*
 * Copyright (C) 2008 - 2012 Timothy M. Jones, Simone Campanoni
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

#ifndef VLLPA_TYPES_H
#define VLLPA_TYPES_H

/**
 * This file contains abstract types for use with the interprocedural pointer
 * analysis.
 */

#include <compiler_memory_manager.h>
#include <jitsystem.h>
#include <xanlib.h>

#include "vllpa_macros.h"
#include "vllpa_memory.h"
#include "vset.h"


/**
 * The types that an unknown initial value can have.
 */
typedef enum { UIV_FIELD, UIV_VAR, UIV_ALLOC, UIV_GLOBAL, UIV_FUNC, UIV_SPECIAL } uiv_type_t;


/**
 * The IDs of special uivs.
 */
typedef enum { UIV_SPECIAL_STDIN = 0, UIV_SPECIAL_STDOUT = 1, UIV_SPECIAL_STDERR = 2 } uiv_special_t;


/**
 * The value of an offset that means access to a whole array.
 */
#define WHOLE_ARRAY_OFFSET JITMAXINT32


/**
 * Unknown initial values have this structure.  The type denotes the parts
 * of the union to use.  There is a high-level link to the base uiv to
 * avoid recursing through the data structure when this is (often) needed.
 */
typedef struct uiv_t {
    uiv_type_t type;
    union {
        struct {
            struct uiv_t *inner;
            JITNINT offset;
        } field;
        struct {
            ir_item_t *param;
            JITNINT offset;
        } global;
        struct {
            ir_method_t *method;
            ir_instruction_t *inst;
        } alloc;
        struct {
            ir_method_t *method;
            JITUINT32 var;
        } var;
        ir_method_t *method;
        uiv_special_t id;
    } value;
    struct uiv_t *base;
} uiv_t;


/**
 * Information required to keep track of abstract addresses.
 */
typedef struct {
    uiv_t *uiv;
    JITNINT offset;
} abstract_addr_t;


/**
 * The types an abstract address merge can have.
 */
typedef enum {AA_MERGE_UIVS = 1, AA_MERGE_OFFSETS = 2} aa_merge_type_t;


/**
 * An abstract address merge type.  This contains enough information to
 * determine whether to merge an abstract address with another.  It is
 * used for global merges.
 */
typedef struct {
    aa_merge_type_t type:2;
    JITNUINT stride:30;
    union {
        uiv_t *uiv;
        abstract_addr_t *to;
    };
} aa_merge_t;


/**
 * In the global and local sets of field uivs, uivs with the same inner uiv
 * are kept together in lists.  However, this means that a lot of lists are
 * created for uivs that are never used as the inner uiv for a field uiv.
 * Therefore, this structure is used when there is only a single uiv in the
 * list and the list created when a second uiv is added, to save space.
 */
typedef struct field_uiv_holder_t {
    JITBOOLEAN isList;
    union {
        uiv_t *uiv;
        XanList *list;
    } value;
} field_uiv_holder_t;


/**
 * The type of an abstract address set in set form.  It has to support fast
 * access to check whether an item exists or not, as well as quick merging
 * between elements.  Hence abstract addresses are placed in a hash table
 * using their uiv's base as a key.  When multiple abstract addresses have
 * the same base uiv, a new hash table is created to hold them all.  Only
 * these abstract addresses can be merged together, meaning that merge should
 * be fast, but there is also at most a two-step lookup to determine whether
 * an abstract address is in the set or not, meaning this should also be fast.
 **/
typedef union {
    abstract_addr_t *aa;
    VSet *vset;
} abs_addr_set_t;

#define ABS_ADDR_SET_EMPTY          0x0
#define ABS_ADDR_SET_SINGLE         0x1
#define ABS_ADDR_SET_SET            0x2
#define ABS_ADDR_SET_VSET           0x3
#define ABS_ADDR_SET_MASK           0x3
#define absAddrSetGetType(s)        ((JITNINT)((s)->aa) & ABS_ADDR_SET_MASK)
#define absAddrSetStripType(s)      ((JITNINT)((s)->aa) & (~ABS_ADDR_SET_MASK))
#define absAddrSetIsEmpty(s)        (absAddrSetGetType(s) == ABS_ADDR_SET_EMPTY)
#define absAddrSetIsSingle(s)       (absAddrSetGetType(s) == ABS_ADDR_SET_SINGLE)
#define absAddrSetIsSet(s)          (absAddrSetGetType(s) == ABS_ADDR_SET_SET)
#define absAddrSetIsVSet(s)         (absAddrSetGetType(s) == ABS_ADDR_SET_VSET)
#define absAddrSetGetSingle(s)      ((abstract_addr_t *)absAddrSetStripType(s))
#define absAddrSetGetSet(s)         ((VSet *)absAddrSetStripType(s))
#define absAddrSetGetVSet(s)        ((VSet *)absAddrSetStripType(s))
#define absAddrSetSetEmpty(s)       ((s)->aa = (abstract_addr_t *)(absAddrSetStripType(s) | ABS_ADDR_SET_EMPTY))
#define absAddrSetSetSingle(s, v)   ((s)->aa = (abstract_addr_t *)((JITNINT)(v) | ABS_ADDR_SET_SINGLE))
#define absAddrSetSetSet(s, v)      ((s)->vset = (VSet *)((JITNINT)(v) | ABS_ADDR_SET_SET))
#define absAddrSetSetVSet(s, v)     ((s)->vset = (VSet *)((JITNINT)(v) | ABS_ADDR_SET_VSET))


/**
 * Types for checking prefixes of abstract address in sets against each other.
 */
typedef enum {
    AASET_PREFIX_NONE,
    AASET_PREFIX_FIRST,
    AASET_PREFIX_SECOND,
    AASET_PREFIX_BOTH
} aaset_prefix_t;


/**
 * Initialise types required for this analysis.
 */
void vllpaInitTypes(void);

/**
 * Free memory used by types for this analysis.
 */
void vllpaFinishTypes(void);

/**
 * Allocate a new unknown initial value.
 */
uiv_t * allocUiv(void);

/**
 * Create a new unknown initial value from another.
 */
uiv_t * newUivFromUiv(uiv_t *inner, JITNINT offset, SmallHashTable *uivTable);

/**
 * Create a new unknown initial value from a register.
 */
uiv_t * newUivFromVar(ir_method_t *method, IR_ITEM_VALUE var, SmallHashTable *uivTable);

/**
 * Create a new unknown initial value from an alloc call.
 */
uiv_t * newUivFromAlloc(ir_method_t *method, ir_instruction_t *inst, SmallHashTable *uivTable);

/**
 * Create a new unknown initial value from a function pointer.
 */
uiv_t * newUivFromFunc(IR_ITEM_VALUE addr, SmallHashTable *uivTable);

/**
 * Create a new unknown initial value from a global.
 */
uiv_t * newUivFromGlobal(ir_item_t *param, JITNINT offset, SmallHashTable *uivTable);

/**
 * Create a new unknown initial value from a global.  This is also called
 * with function pointer parameters, so a check distinguishes between
 * them and selects the correct one to use.
 */
uiv_t * newUivFromGlobalParam(ir_item_t *param, SmallHashTable *uivTable);

/**
 * Create a new unknown initial value based on the given unknown initial
 * value parameter.  This is useful for creating a new uiv within 'uivTable'
 * when it doesn't already exist.  For example, within a different method.
 */
uiv_t * newUivBasedOnUiv(uiv_t *uiv, SmallHashTable *uivTable);

/**
 * Create a new special unknown initial value using the given ID.
 */
uiv_t * newUivSpecial(uiv_special_t id, SmallHashTable *uivTable);

/**
 * Return JITTRUE if the given uiv is used within the method that contains
 * the given uiv table.
 */
JITBOOLEAN uivExistsInTable(uiv_t *uiv, SmallHashTable *uivTable);

/**
 * Free an unknown initial value.
 */
void freeUivLevel(uiv_t *uiv);

/**
 * Get the base unknown initial value within the given unknown initial
 * value.
 */
uiv_t * getBaseUiv(uiv_t *uiv);

/**
 * Get the nesting level of the base unknown initial value within the
 * given unknown initial value.
 */
JITUINT32 getLevelOfBaseUiv(uiv_t *uiv);

/**
 * Get the offset associated with the field uiv at the given level from
 * within the given unknown initial value.  Returns -1 if the given uiv
 * is not a field type.
 */
JITINT32 getFieldOffsetAtLevelFromUiv(uiv_t *uiv, JITUINT32 level);

/**
 * Get the unknown initial value within the given unknown initial value
 * at the given nesting level.
 */
uiv_t * getUivAtLevel(uiv_t *uiv, JITUINT32 level);

/**
 * Get a set of uivs that make up the given uiv.
 */
VSet * getConstituentUivSet(uiv_t *uiv);

/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a variable type.
 */
JITBOOLEAN isBaseUivFromVar(uiv_t *uiv);

/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a function pointer type.
 */
JITBOOLEAN isBaseUivFromFunc(uiv_t *uiv);

/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a global type.
 */
JITBOOLEAN isBaseUivFromGlobal(uiv_t *uiv);

/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a global type from a symbol.
 */
JITBOOLEAN isBaseUivFromGlobalSymbol(uiv_t *uiv);

/**
 * Get the variable from the base unknown initial value within the given
 * unknown initial value.  Does no checking but assumes that the base is
 * definitely a variable type.
 */
JITUINT32 getVarFromBaseUiv(uiv_t *uiv);

/**
 * Get the instruction that allocates memory from the base unknown initial
 * value within the given unknown initial value.  Does no checking but
 * assumes that the base is definitely an alloc type.
 */
ir_instruction_t * getInstFromBaseUiv(uiv_t *uiv);

/**
 * Get the parameter that represents the global from the base unknown initial
 * value within the given unknown initial value.  Does no checking but assumes
 * that the base is definitely a global type.
 */
ir_item_t * getParamFromGlobalBaseUiv(uiv_t *uiv);

/**
 * Check whether prefix is a prefix of uiv.
 */
JITBOOLEAN isUivAPrefix(uiv_t *prefix, uiv_t *uiv);


/**
 * Print a single unknown initial value.
 */
#ifdef PRINTDEBUG
void printUiv(uiv_t *uiv, char *prefix, JITBOOLEAN newline);
#else
#define printUiv(uiv, prefix, newline)
#endif

/**
 * Create a new abstract address with the given parameters.
 */
abstract_addr_t * newAbstractAddress(uiv_t *uiv, JITNINT off);

/**
 * Create a new abstract address set that is in empty form.
 */
abs_addr_set_t * newAbstractAddressSetEmpty(void);

/**
 * Create a new abstract address set containing one abstract address.
 */
abs_addr_set_t * newAbstractAddressSet(abstract_addr_t *aa);

/**
 * Create a new abstract address set containing a single new abstract
 * address made up from the given parameters.
 */
abs_addr_set_t * newAbstractAddressInNewSet(uiv_t *uiv, JITNINT offset);

/**
 * Get the first abstract address in a set.
 */
abstract_addr_t * absAddrSetFirst(abs_addr_set_t *set);

/**
 * Get the next abstract address in a set.
 */
abstract_addr_t * absAddrSetNext(abs_addr_set_t *set, abstract_addr_t *prev);

/**
 * Get the number of abstract addresses in an abstract address set.
 */
JITUINT32 absAddrSetSize(abs_addr_set_t *set);

/**
 * Share an abstract address between two or more owners.
 */
abs_addr_set_t * shareAbstractAddressSet(abs_addr_set_t *aaSet);

/**
 * Clone a set of abstract addresses.  Basically create a new set with
 * the same abstract addresses in the new as the old.
 */
abs_addr_set_t * absAddrSetClone(abs_addr_set_t *aaSet);

/**
 * Clone a set of abstract addresses without a given abstract address.
 */
abs_addr_set_t * absAddrSetCloneWithoutAbsAddr(abs_addr_set_t *aaSet, abstract_addr_t *aaRemove);

/**
 * Determine whether two lists of abstract addresses are identical.  This
 * means that all elements in one list are in the other too, and
 * vice-versa.
 */
JITBOOLEAN abstractAddressSetsAreIdentical(abs_addr_set_t *first, abs_addr_set_t *second);

/**
 * Determine whether an abstract address exists within a set.
 */
JITBOOLEAN  absAddrSetContains(abs_addr_set_t *set, abstract_addr_t *aa);

/**
 * Empty an abstract address set.
 */
void absAddrSetEmpty(abs_addr_set_t *set);

/**
 * Free an abstract address.
 */
void freeAbstractAddress(abstract_addr_t *aa);

/**
 * Free a set of abstract addresses.  Only frees the memory used by the set
 * and leaves the abstract address alone.
 */
void freeAbstractAddressSet(abs_addr_set_t *aaSet);

/**
 * Check to see whether an abstract address would be merged into the given set
 * of abstract addresses.
 */
JITBOOLEAN absAddrSetContainsAbsAddr(abs_addr_set_t *aaSet, abstract_addr_t *aaToMerge);

/**
 * Check to see whether the first set of abstract addresses contains all
 * of the abstract addresses in the second set, after merging.
 */
JITBOOLEAN abstractAddressSetContainsSet(abs_addr_set_t *aaFirstSet, abs_addr_set_t *aaSecondSet);

/**
 * Return JITTRUE if the abstract address could represent a function
 * pointer.
 */
JITBOOLEAN isAbstractAddressAFunctionPointer(abstract_addr_t *aa);

/**
 * Append a single abstract address to a set.  This also performs a merge
 * on the final set if the given parameter is JITTRUE.  Duplicates are
 * automatically removed from the final set too.  Returns JITTRUE if the
 * abstract address is appended and not merged.
 */
void absAddrSetAppendAbsAddr(abs_addr_set_t *aaSet, abstract_addr_t *aa);

/**
 * Append a set of abstract addresses to another set.  This also performs
 * a merge on the final set if the given parameter is JITTRUE.  Duplicates
 * are automatically removed from the final set too.  Returns JITTRUE if
 * any additions or subtractions are made to the first set.  However,
 * items can be reordered.
 */
JITBOOLEAN absAddrSetTrackAppendSet(abs_addr_set_t *aaSet, abs_addr_set_t *toAppend);

/**
 * Append a set of abstract addresses to another set but don't track
 * whether changes have been made.  This also performs a merge on the final
 * set if the given parameter is JITTRUE.  Duplicates are automatically
 * removed from the final set too if required.
 */
void absAddrSetAppendSet(abs_addr_set_t *aaSet, abs_addr_set_t *toAppend);

/**
 * Determine whether two sets of abstract addresses share at least one abstract
 * address.  With the 'checkMerge' option, if an abstract address in one set
 * would merge into an abstract address in the other set, then consider that as
 * an overlap.  With the 'checkPrefixNum' option, check whether an abstract
 * address in one set could be used as a prefix to an abstract address in the
 * other set.  When set to 1, consider only the first abstract address set,
 * but set to 2, check both sets.
 */
JITBOOLEAN abstractAddressSetsOverlap(abs_addr_set_t *first, abs_addr_set_t *second, JITBOOLEAN checkMerge, aaset_prefix_t checkPrefixNum);

/**
 * Return a set of elements that are in both given sets of abstract
 * addresses.  Actually returns the set using the conditions for the
 * function abstractAddressSetsOverlap above.
 */
abs_addr_set_t * getOverlapSetFromAbstractAddressSets(abs_addr_set_t *first, abs_addr_set_t *second);

/**
 * Add a generic merge to target a specific abstract address with a given
 * stride.
 */
void addGenericAbsAddrOffsetMerge(abstract_addr_t *aaFinal, JITUINT32 stride, SmallHashTable *mergeMap);

/**
 * Add a generic merge to target a specific abstract address from one uiv to
 * another.
 */
void addGenericAbsAddrUivMerge(uiv_t *from, uiv_t *to, SmallHashTable *mergeMap);

/**
 * Add a generic mapping between an abstract address that should be merged
 * away and the abstract address it was merged to within the given merge
 * map.  For abstract addresses with the same uivs, the stride between
 * their offsets is calculated and a new abstract address is created that
 * has the lowest positive stride as its offset.  This is the final
 * abstract and is returned to the caller.  Otherwise, no merge is created
 * and NULL is returned.
 */
abstract_addr_t * addGenericAbsAddrMergeMapping(abstract_addr_t *aaFrom, abstract_addr_t *aaTo, SmallHashTable *mergeMap);

/**
 * Record a uiv as being a merge target.
 */
void addUivMergeTarget(SmallHashTable *targets, uiv_t *uiv, aa_merge_type_t type);

/**
 * Merge abstract addresses with the same base unknown initial value and
 * offsets, and those that have the same unknown initial value but
 * different offsets.  Uivs that are the targets of a merge are placed in the
 * given hash table, if present.
 */
void absAddrSetMerge(abs_addr_set_t *aaSet, SmallHashTable *uivMergeTargets);

/**
 * Check for merge points between two sets of abstract addresses.  Return
 * a set of abstract addresses that consists of the first set with any
 * merges that were found.  Uivs that are the targets of a merge are placed in
 * the given hash table, if present.
 */
abs_addr_set_t * absAddrSetMergeBetweenSets(abs_addr_set_t *first, abs_addr_set_t *second, SmallHashTable *uivMergeTargets);

/**
 * Apply the given generic merge map to each abstract address in the given
 * set.
 */
void applyGenericMergeMapToAbstractAddressSet(abs_addr_set_t *aaSet, SmallHashTable *mergeMap);

/**
 * Add an abstract address to a list keeping it sorted.
 */
void addAbsAddrToOrderedList(XanList *list, abstract_addr_t *aaAppend);

/**
 * Create an ordered list of abstract addresses from a set.
 */
XanList * absAddrSetToUnorderedList(abs_addr_set_t *aaSet);

/**
 * Create an array of abstract addresses from a set.
 */
abstract_addr_t ** absAddrSetToArray(abs_addr_set_t *aaSet, JITUINT32 *num);

/**
 * Print a single abstract address.
 */
#ifdef PRINTDEBUG
void printAbstractAddress(abstract_addr_t *aa, JITBOOLEAN newline);
#else
#define printAbstractAddress(aa, newline)
#endif

/**
 * Print the abstract addresses from the given list.
 */
#ifdef PRINTDEBUG
void printAbstractAddressList(XanList *aaList);
#else
#define printAbstractAddressList(aaList)
#endif

/**
 * Print the abstract addresses from the given set.
 */
#ifdef PRINTDEBUG
void printAbstractAddressSet(abs_addr_set_t *aaSet);
#else
#define printAbstractAddressSet(aaSet)
#endif

/**
 * Check an unknown initial value to ensure it is sane.
 */
#if defined(DEBUG) && (defined(CHECK_ABSTRACT_ADDRESSES) || defined(CHECK_UIVS))
void checkUiv(uiv_t *uiv);
#else
#define checkUiv(uiv)
#endif

/**
 * Check two unknown initial values to see if they match in terms of
 * their structure fields.  Returns JITTRUE if they match.
 */
#if defined(DEBUG) && (defined(CHECK_ABSTRACT_ADDRESSES) || defined(CHECK_UIVS))
JITBOOLEAN checkWhetherUivsMatch(uiv_t *first, uiv_t *second);
#else
#define checkWhetherUivsMatch(first, second)
#endif

/**
 * Print the memory usage of the types module.
 */
#if defined(MEMUSE_DEBUG)
void vllpaTypesPrintMemUse(void);
#else
#define vllpaTypesPrintMemUse()
#endif  /* if defined(MEMUSE_DEBUG) */

#endif  /* VLLPA_TYPES_H */
