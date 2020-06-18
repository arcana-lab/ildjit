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

#ifndef VLLPA_INFO_H
#define VLLPA_INFO_H

/**
 * This file contains types and prototypes for manipulating the methods
 * information structure for the interprocedural pointer analysis.
 */

#include <compiler_memory_manager.h>
#include <xanlib.h>

#include "vllpa_macros.h"
#include "vllpa_memory.h"
#include "vllpa_types.h"
#include "vset.h"


/**
 * Information required for Tarjan's stronly connected components algorithm.
 */
typedef struct {
    int index;
    int lowlink;
} tarjan_scc_t;


/**
 * Different types of call that are recognised.
 */
typedef enum {
    NORMAL_CALL,
    LIBRARY_CALL,
    OPEN_CALL,
    TMPFILE_CALL,
    TIME_CALL,
    CTIME_CALL,
    DIFFTIME_CALL,
    ERRNO_CALL,
    REMOVE_CALL,
    CLOSE_CALL,
    READ_CALL,
    WRITE_CALL,
    FEOF_CALL,
    FGETS_CALL,
    FREAD_CALL,
    FGETC_CALL,
    SSCANF_CALL,
    FSCANF_CALL,
    SPRINTF_CALL,
    FPRINTF_CALL,
    FOPEN_CALL,
    FCLOSE_CALL,
    FSEEK_CALL,
    FILENO_CALL,
    FWRITE_CALL,
    FPUTC_CALL,
    FFLUSH_CALL,
    REWIND_CALL,
    SETJMP_CALL,
    SETBUF_CALL,
    ATOI_CALL,
    GETENV_CALL,
    CLOCK_CALL,
    IO_FTABLE_GET_ENTRY_CALL,
    EXIT_CALL,
    MAXMIN_CALL,
    ABS_CALL,
    PERROR_CALL,
    STRERROR_CALL,
    STRCAT_CALL,
    STRCPY_CALL,
    STRRCHR_CALL,
    STRCMP_CALL,
    STRSTR_CALL,
    STRPBRK_CALL,
    STRTOD_CALL,
    RAND_CALL,
    TOLOWER_CALL,
    TOUPPER_CALL,
    ISALNUM_CALL,
    ISSPACE_CALL,
    ISALPHA_CALL,
    ISUPPER_CALL,
    FPUTS_CALL,
    PRINTF_CALL,
    PUTS_CALL,
    PUTCHAR_CALL,
    QSORT_CALL,
    CALLOC_CALL,
    MALLOC_CALL,
    BUFFER_FLUSH_CALL,
    BUFFER_WRITEBITS_CALL,
    BUFFER_WRITEBITSFROMVALUE_CALL,
    BUFFER_GETNUMBEROFBYTESWRITTEN,
    BUFFER_SETNUMBEROFBYTESWRITTEN,
    BUFFER_READBITS,
    BUFFER_NEEDTOREADNUMBEROFBITS,
    BUFFER_SETSTREAM,
} call_type_t;

/**
 * A call site type.  A call site is bound to a call instruction and
 * contains a pointer to the original method that is called and a hash
 * table that maps the uivs in the callee method to sets of caller
 * abstract addresses.  The memMapNum refers to the callee's memory map
 * to use.
 */
typedef struct {
    JITBOOLEAN sameSCC;
    call_type_t callType;
    ir_method_t *method;
} call_site_t;


/**
 * A mapping to an abstract address set.  All mappings to abstract address
 * sets target this structure.  The updated fields determine
 * whether the set has been updated since it was last reset.  By checking
 * these when transferring abstract addresses between methods, unnecessary
 * work can be saved.  The aaOriginalSet is a copy of the aaSet at the start
 * of a fixed point iteration.  At the end of the iteration, when abstract
 * addresses are added to the main set, the main set is merged and compared
 * with the original set to determine whether any changes have been made.
 * There is also a merge map that records all the merges that have been
 * made on this particular set, to avoid situations where an abstract
 * address gets added to the set and should be merged away, but can't be
 * until another (intermediate) abstract address is added.
 */
typedef struct {
    abs_addr_set_t *aaSet;
    abs_addr_set_t *aaSuperSet;
    abs_addr_set_t *aaOriginalSet;
    JITUINT32 numOriginalAddrs;
    abstract_addr_t **originalAbsAddrs;
} aa_mapped_set_t;


/**
 * All generated data about each method.
 * - ssaMethod: the method in SSA form.
 * - fieldInsts: list of instructions that perform structure field accesses.
 * - instMap: mapping between instructions in SSA method and original method.
 * - callInsts: list of call instructions within this method.
 * - callMethodMap: mapping between call instructions and a list of call
 *   sites for non-library methods called.
 * - containsLibraryCall: whether this method or a later callee actually
 *   calls a library method.
 * - containsICall: whether this method or a later callee actually contains
 *   an indirect call.
 * - tarjan: information for Tarjan's SCC algorithm.
 * - uivs: the unknown initial values used by the method.
 * - uivDefMap: the set of instructions within the method that define each
 *   base UIV_ALLOC uiv that is used within the method.
 * - absAddrs: the abstract addresses used by the method.
 * - varAbsAddrSets: the set of abstract addresses each register variable
 *   can point to.
 * - memAbsAddrMap: the set of abstract addresses each memory location can
 *   point to.
 * - mergeAbsAddrMap: mapping between uivs and a set of abstract address
 *   merges that have been made.
 * - readSet: the set of abstract addresses the method reads.
 * - writeSet: the set of abstract addresses the method writes.
 * - callReadMap: mapping between call instructions and the set of abstract
 *   addresses read by all callees.
 * - callWriteMap: mapping between call instructions and the set of
 *   abstract addresses written by all callees.
 * - readInsts: a map between abstract addresses read and the instructions
 *   within the (original) method that read them.
 * - writeInsts: a map between abstract addresses written and the
 *   instructions within the (original) method that write them.
 * - numCallers: the number of callers (call instructions) that target this
 *   method.
 * - callersProcessed: the number of callers (call instructions) that have
 *   taken their information from this method already.
 * - funcPtrAbsAddrMap: a map between call instructions (not necessarily
 *   from this method) and further maps of abstract addresses and the
 *   set of function pointers they could point to.
 * - changedFromAnalysis: whether this method's memory or returned
 *   variables changed in the last analysis pass.
 */
typedef struct method_info_t {
    ir_method_t *ssaMethod;
    VSet *fieldInsts;
    SmallHashTable *instMap;
    XanList *callInsts;                         /**< A list of call instructions in the method, ordered by instruction ID. */
    SmallHashTable *callMethodMap;
    JITBOOLEAN containsLibraryCall;
    JITBOOLEAN containsICall;
    tarjan_scc_t *tarjan;
    SmallHashTable *uivs;
    SmallHashTable *uivDefMap;
    aa_mapped_set_t **varAbsAddrSets;
    SmallHashTable *memAbsAddrMap;
    SmallHashTable *mergeAbsAddrMap;
    SmallHashTable *uivToMemAbsAddrKeysMap;
    abs_addr_set_t *readSet;
    abs_addr_set_t *writeSet;
    SmallHashTable *callReadMap;
    SmallHashTable *callWriteMap;
    SmallHashTable *readInsts;
    SmallHashTable *writeInsts;
    JITUINT32 numCallers;
    JITUINT32 callersProcessed;
    SmallHashTable *funcPtrAbsAddrMap;
    JITBOOLEAN changedFromAnalysis;
    abs_addr_set_t *initialAbsAddrTransferSet;
    SmallHashTable *calleeUivMaps;
    SmallHashTable *calleeToCallerAbsAddrMap;   /**< Cache mapping callee abstract addresses to caller abstract address sets. */
    VSet *cachedUivMapTransfers;                /**< Cache of caller abstract addresses mapped from callees. */
    VSet *cachedMemAbsAddrTransfers;            /**< Cache of caller abstract addresses mapped from callees. */
    JITBOOLEAN memAbsAddrsModified;             /**< Whether this method's memory abstract addresses were modified on the last analysis iteration. */
    SmallHashTable *uivMergeTargets;            /**< Set of uivs that are the targets of merges, mapping to the types of merges. */
    VSet *emptyPossibleCallees;                 /**< Set of possible callees that don't have instructions. */
} method_info_t;


/**
 * Create a mapping of an abstract address set with the given number of
 * entries in the updated array.
 */
aa_mapped_set_t * createAbsAddrMappedSet(JITUINT32 numUpdates);

/**
 * Free a mapped abstract address set.
 */
void freeAbsAddrMappedSet(aa_mapped_set_t *aaMappedSet);

/**
 * Copy each abstract address set in the given mapping table so that later,
 * after appending abstract addresses and merging, each new set can be
 * compared to the old one to determine whether any changes have been made.
 */
void copyMappedAbsAddrSetsTable(SmallHashTable *mapToAbsAddrSet);

/**
 * Copy each mapped abstract address set in the given array.
 */
void copyMappedAbsAddrSetsArray(aa_mapped_set_t **arrayOfAbsAddrSets, JITUINT32 num);

/**
 * Merge the abstract addresses in all abstract address sets in the given
 * mapping table.
 */
void mergeMappedAbsAddrSetsTable(SmallHashTable *mapToAbsAddrSet, SmallHashTable *mergeMap);

/**
 * Merge each mapped abstract address set in the given array.
 */
void mergeMappedAbsAddrSetsArray(aa_mapped_set_t **arrayOfAbsAddrSets, JITUINT32 num, SmallHashTable *uivMergeTargets);

/**
 * Check each abstract address set in the given mapping table against the
 * orginal copy that was previously made to determine whether any changes
 * have occurred.  At the same time, free all of the original copies.
 */
JITBOOLEAN haveMappedAbsAddrSetsChangedTable(SmallHashTable *mapToAbsAddrSet, JITBOOLEAN fromAbsAddr);

/**
 * Check each mapped abstract address set in the given array for changes.
 */
JITBOOLEAN haveMappedAbsAddrSetsChangedArray(aa_mapped_set_t **arrayOfAbsAddrSets, JITUINT32 num);

/**
 * Get a set of abstract addresses that the given memory location
 * (abstract address) can point to within the given method.  If there
 * isn't a set for this location already then create one.  Returns
 * a pointer to the set for this location, so that it can be altered
 * if necessary.
 */
abs_addr_set_t * getMemAbstractAddressSet(abstract_addr_t *aa, method_info_t *info, JITBOOLEAN create);

/**
 * Get a set of abstract addresses that the given variable can point to
 * within the given method.  Creates a new set if the there isn't one
 * already saved.  If the variable is an escaped variable then the actual
 * set of abstract addresses will be in memory, so look there instead.
 * This version takes the variable directly as an argument.
 */
abs_addr_set_t * getVarAbstractAddressSetVarKey(JITUINT32 var, method_info_t *info, JITBOOLEAN now);

/**
 * Get a set of abstract addresses that the given variable can point to
 * within the given method.  Creates a new set if the there isn't one
 * already saved.  If the variable is an escaped variable then the actual
 * set of abstract addresses will be in memory, so look there instead.
 */
abs_addr_set_t * getVarAbstractAddressSet(ir_item_t *param, method_info_t *info, JITBOOLEAN now);

/**
 * Get the set of abstract addresses that a load or store instruction accesses.
 * I.e. those that are keys into the memory abstract address map.  The returned
 * set needs freeing after use.
 */
abs_addr_set_t * getLoadStoreAccessedAbsAddrSet(ir_instruction_t *inst, method_info_t *info);

/**
 * Get the set of abstract addresses accessed through a call parameter.
 */
abs_addr_set_t * getCallParamAccessedAbsAddrSet(ir_instruction_t *inst, JITUINT32 nth, method_info_t *info);

/**
 * Get the set of abstract addresses accessed through a call parameter, along
 * with all others that are reachable from these addresses too.
 **/
abs_addr_set_t * getCallParamReachableAbsAddrSet(ir_instruction_t *inst, JITUINT32 nth, method_info_t *info);

/**
 * Get the set of abstract addresses accessed through an instruction parameter.
 */
abs_addr_set_t * getInstParamAccessedAbsAddrSet(ir_instruction_t *inst, JITUINT32 parNum, method_info_t *info);

/**
 * Append the given set of abstract addresses to the set that the given
 * memory location (abstract address) can point to within the given
 * method.  Will not free the given set of abstract addresses.
 */
void appendSetToMemAbsAddrSet(abstract_addr_t *aaKey, method_info_t *info, abs_addr_set_t *set);

/**
 * Set the set of abstract addresses that the given variable can point
 * to within the given method.  This actually stores the given set so any
 * changes made after a call to this function will be reflected in the
 * set stored for this variable.  If the given variable is an escaped
 * variable then the set will be stored in memory instead.  Returns JITTRUE
 * if there is a change in the stored set (i.e. any of the set items are
 * different, or a new set is stored).
 */
JITBOOLEAN setVarAbstractAddressSet(JITUINT32 var, method_info_t *info, abs_addr_set_t *set, JITBOOLEAN replace);

/**
 * Map a callee abstract address to a set of caller abstract addresses.
 */
abs_addr_set_t * mapCalleeAbsAddrToCallerAbsAddrSet(abstract_addr_t *aa, method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap, JITBOOLEAN expandVarMappings);

/**
 * Map a set of callee abstract addresses to a set of caller abstract
 * addresses, returning the set of mapped abstract addresses.
 */
abs_addr_set_t * mapCalleeAbsAddrSetToCallerAbsAddrSet(abs_addr_set_t *calleeSet, method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap);

/**
 * Determine whether an abstract address is a key into the given abstract
 * address map.
 */
JITBOOLEAN isKeyAbsAddr(abstract_addr_t *aa, SmallHashTable *absAddrMap);

/**
 * Get a set of abstract addresses that are used as keys into the memory
 * abstract addresses for the given abstract address.
 */
abs_addr_set_t * getKeyAbsAddrSetFromAbsAddr(abstract_addr_t *aa, method_info_t *info);

/**
 * Get a set of abstract addresses that are used as keys into the memory
 * abstract addresses, where the keys contain the given uiv.
 */
abs_addr_set_t * getKeyAbsAddrSetFromUivKey(uiv_t *uiv, method_info_t *info);

/**
 * Get a set of abstract addresses that are used as keys into the given
 * abstract address map when they have a prefix of the given abstract
 * address.
 */
abs_addr_set_t * getKeyAbsAddrSetFromUivPrefix(uiv_t *uivPrefix, method_info_t *info);

/**
 * Get a set of abstract addresses that are keys into memory based on merges
 * that could have led to the given abstract address.  In other words, the uiv
 * that makes up the abstract address, and its components if it is a field uiv,
 * could have been the result of a merge.  If that is the case, we need to
 * build a set of abstract addresses that are keys into memory and could have
 * been constructed had that merge not taken place.
 */
abs_addr_set_t * getMemKeyAbsAddrSetFromMergedUivKey(uiv_t *uiv, method_info_t *info);

/**
 * Add concrete values to a single set.
 */
void addConcreteValuesToAbsAddrSet(abs_addr_set_t *aaSet, method_info_t *info, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap);

/**
 * Check whether the given set of abstract addresses would merge into any
 * of the target sets of memory abstract addresses that have a key
 * containing the given uiv.  If the set would be merged into any of the
 * current sets in memory then it may not be added in a later stage, which
 * can avoid problems of continually adding mappings and never terminating.
 * Returns JITTRUE if the given set would merge into any of the existing
 * sets.
 */
JITBOOLEAN doesAbsAddrSetMergeIntoMemSet(uiv_t *uivKey, abs_addr_set_t *aaSet, method_info_t *info);

#endif  /* VLLPA_INFO_H */
