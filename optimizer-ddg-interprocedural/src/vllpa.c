/*
 * Copyright (C) 2009 - 2012 Timothy M Jones, Simone Campanoni
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
 * This file implements the VLLPA algorithm from the following paper:
 *
 * Practical and Accurate Low-Level Pointer Analysis.  Bolei Guo, Matthew
 * J. Bridges, Spyridon Triantafyllis, Guilherme Ottoni, Easwaran Raman
 * and David I. August.  Proceedings of the third International Symposium
 * on Code Generation and Optimization (CGO), March 2005.
 *
 * It also makes use of the Strongly Connected Components algorithm from:
 *
 * Depth-first search and linear graph algorithms.  Robert E. Tarjan.
 * SIAM Journal on Computing 1(2), pages 146-160, 1972.
 */

/* This is for strsignal in string.h. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <compiler_memory_manager.h>
#include <execinfo.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>
#include <xanlib.h>

// My headers
#include "optimizer_ddg.h"
#include "smalllib.h"
#include "vllpa.h"
#include "vllpa_aliases.h"
#include "vllpa_info.h"
#include "vllpa_macros.h"
#include "vllpa_memory.h"
#include "vllpa_types.h"
#include "vllpa_util.h"
#include "vset.h"
// End


#ifdef CALLGRIND_PROFILE
#include <valgrind/callgrind.h>
#define startInstrumentation() CALLGRIND_START_INSTRUMENTATION
#define stopInstrumentation() CALLGRIND_STOP_INSTRUMENTATION
#define dumpInstrumentation() CALLGRIND_DUMP_STATS
#else
#define startInstrumentation()
#define stopInstrumentation()
#define dumpInstrumentation()
#endif  /* ifdef CALLGRIND_PROFILE */


/**
 * A mapping to a set of function pointer abstract addresses for use when
 * resolving function pointers.  If the pointer boolean is JITTRUE then
 * the abstract address that maps to this set is a pointer to a set of
 * function pointers.  Otherwise it represents a function pointer (i.e.
 * it is a variable that can be assigned a function pointer).
 */
typedef struct {
    JITBOOLEAN pointer;
    method_info_t *info;
    abs_addr_set_t *funcPtrSet;
} func_ptr_resolve_set_t;


/**
 * This is used to set an iteration count to loops that iterate until a
 * fixed point.  After the iteration count is complete they abort with an
 * error.  If debug printing is enabled, a couple of iterations before the
 * abort printing is turned on to facilitate debugging.
 */
#ifdef PRINTDEBUG
#define FIXED_POINT_ITER_START                          \
    JITUINT32 fixedPointIterNum = 0;                    \
    JITBOOLEAN fixedPointIterOldPDebug = enablePDebug;

#define FIXED_POINT_ITER_NEW(maxIter, name)                     \
    ++fixedPointIterNum;                                        \
    PDEBUGNL("Iteration %u of %s\n", fixedPointIterNum, name);  \
    if (fixedPointIterNum > maxIter - 5) {                      \
        SET_DEBUG_ENABLE(JITTRUE);                              \
    }                                                           \
    if (fixedPointIterNum > maxIter) {                          \
        abort();                                                \
    }
#define FIXED_POINT_ITER_END                    \
    SET_DEBUG_ENABLE(fixedPointIterOldPDebug);
#else
#define FIXED_POINT_ITER_START                  \
    JITUINT32 fixedPointIterNum = 0;
#define FIXED_POINT_ITER_NEW(maxIter, name)                     \
    ++fixedPointIterNum;                                        \
    if (fixedPointIterNum > maxIter) {                          \
        abort();                                                \
    }
#define FIXED_POINT_ITER_END
#endif



/**
 * This calculates the time taken by various stages of the analysis and
 * prints them out at the end, if required.
 */
#ifdef DEBUG_TIMINGS
/**
 * Number of abstract address set transfers from callee to caller avoided.
 */
static JITUINT64 avoidedTransfers = 0;
#define avoidedTransfer() ++avoidedTransfers
/**
 * Time (in micro-seconds) spent in each phase of the analysis.
 */
static JITUINT64 analysisTime = 0;
static JITUINT64 pointsToTime = 0;
static JITUINT64 transferTime = 0;
static JITUINT64 funcPtrTime = 0;
static JITUINT64 sccTime = 0;
static JITUINT64 initTransferTime = 0;
static JITUINT64 augmentUivMapTime = 0;
static JITUINT64 augmentMemMapTime = 0;
static JITUINT64 aliasTime = 0;
static JITUINT64 memBaseTime = 0;
static JITUINT64 vllpaTime = 0;
#define newTimer(timer) struct timeval timer
#define setTimer(tv) gettimeofday(tv, NULL)
#define getMicroSeconds(tv) (tv.tv_sec * 1000000 + tv.tv_usec)
#define updateTiming(timer, start, end) timer += getMicroSeconds(end) - getMicroSeconds(start)
#else
#define avoidedTransfer()
#define newTimer(timer)
#define setTimer(tv)
#define updateTiming(timer, start, end)
#endif  /* ifdef DEBUG_TIMINGS */


/**
 * Don't set outside instructions in dependences for now.
 */
const JITBOOLEAN computeOutsideInsts = JITFALSE;


/**
 * Whether to resolve function pointers or not.  Set through the environment
 * variable 'DDG_IP_CALCULATE_FUNCTION_POINTERS'.
 */
static JITBOOLEAN calculateFunctionPointers = JITTRUE;


/**
 * Whether to print some statistics about the program before analysis.
 */
static JITBOOLEAN printProgramStatistics = JITFALSE;


/**
 * Whether to use information about known libary calls or not.  Set through
 * the environment variable 'DDG_IP_USE_KNOWN_CALLS'.
 */
static JITBOOLEAN useKnownCalls = JITTRUE;


/**
 * Whether to use information contained within the type_infos fields of
 * parameters or not.  Set through the environment variable
 * 'DDG_IP_USE_TYPE_INFOS'.
 */
JITBOOLEAN useTypeInfos = JITTRUE;


/**
 * Level for dumping CFG.
 */
extern JITUINT32 ddgDumpCfg;


/**
 * Name of specific function for extended debug printing.
 */
char *extendedPrintDebugFunctionName = NULL;


/**
 * Statistics about memory data dependences identified.  All pairs and pairs
 * between unique instructions.
 */
JITUINT32 memoryDataDependencesAll;
JITUINT32 memoryDataDependencesInst;


/**
 * Do any global initialisation.
 */
static inline void
initGlobals(void) {
    memoryDataDependencesAll = 0;
    memoryDataDependencesInst = 0;
    if (computeOutsideInsts) {
        fprintf(stderr, "Warning: Computing outside instructions uses a lot of memory and needs to be optimised\n");
    }
}


/**
 * Create a new store instruction before the given instruction to store
 * the non-variable parameter number given to a new variable and adjust
 * the original instruction to use the new variable.
 */
static void
moveParamIntoStore(ir_method_t *method, ir_instruction_t *inst, JITUINT8 paramNum) {
    ir_instruction_t *move;
    ir_item_t *param = IRMETHOD_getInstructionParameter(inst, paramNum);

    /* Create a new move instruction. */
    move = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRMOVE);

    /* Copy the pointer into the move source. */
    IRMETHOD_cpInstructionParameter1(move, param);

    /* Create a new result variable and update the original instruction. */
    IRMETHOD_setInstructionParameterWithANewVariable(method, inst, param->type, NULL, paramNum);

    /* Copy the new variable into the move result. */
    IRMETHOD_cpInstructionResult(move, param);
}


/**
 * Create a new store instruction before the given instruction if the
 * given parameter number for that instruction contains a pointer.  The
 * new store will simply store the non-variable to a new variable and
 * adjust the original instruction to use the new variable.  Returns
 * JITTRUE if a new store was created.
 */
static JITBOOLEAN
movePointerParamIntoStore(ir_method_t *method, ir_instruction_t *inst, JITUINT8 paramNum) {
    JITBOOLEAN added = JITFALSE;
    ir_item_t *param = IRMETHOD_getInstructionParameter(inst, paramNum);

    /* Check for a non-variable parameter to move. */
    if (paramIsGlobalPointer(param)) {
        moveParamIntoStore(method, inst, paramNum);
        added = JITTRUE;
    }

    /* Return whether a store was added. */
    return added;
}


/**
 * Alter an add to an explicit pointer so that it is a stored to a new
 * variable and an add to that variable.  Returns JITTRUE if at least
 * one store was added.
 */
static JITBOOLEAN
moveAddPointersIntoStores(ir_method_t *method, ir_instruction_t *inst) {
    JITBOOLEAN added = JITFALSE;
    ir_item_t *param1 = IRMETHOD_getInstructionParameter1(inst);
    ir_item_t *param2 = IRMETHOD_getInstructionParameter2(inst);

    /* Can't be a global symbol added to constant. */
    if (!areSymbolAndImmIntParams(param1, param2)) {
        JITUINT32 p;

        /* Check both parameters. */
        for (p = 1; p < 3; ++p) {
            added |= movePointerParamIntoStore(method, inst, p);
        }
    }

    /* Return whether a store was added. */
    return added;
}


/**
 * Alter a call instruction that has an explicit pointer as one of its
 * call parameters so that it is stored to a new variable and that variable
 * used in the call instead.  Returns JITTRUE if at least one pointer was
 * moved.
 */
static JITBOOLEAN
moveCallPointersIntoStores(ir_method_t *method, ir_instruction_t *inst) {
    JITBOOLEAN added = JITFALSE;
    JITUINT32 c;

    /* Check all call parameters. */
    for (c = 0; c < IRMETHOD_getInstructionCallParametersNumber(inst); ++c) {
        ir_item_t *param = IRMETHOD_getInstructionCallParameter(inst, c);

        /* Must be a global pointer. */
        if (paramIsGlobalPointer(param)) {
            IR_ITEM_VALUE newVar = IRMETHOD_newVariableID(method);
            ir_instruction_t *move = IRMETHOD_newInstructionBefore(method, inst);
            IRMETHOD_setInstructionType(method, move, IRMOVE);

            /* Copy the pointer into the move source. */
            IRMETHOD_cpInstructionParameter1(move, param);

            /* Update the original parameter with the new variable. */
            IRMETHOD_setInstructionCallParameter(method, inst, newVar, 0.0, IROFFSET, param->type, NULL, c);

            /* Copy the new variable into the move result. */
            IRMETHOD_cpInstructionResult(move, param);
            added = JITTRUE;
        }
    }

    /* Return whether a store was added. */
    return added;
}


/**
 * Add explicit instructions before each load or store with a non-zero
 * offset that place the base + offset calculation in an explicit add
 * instruction.  This simplifies the later stages of the algorithm which
 * expects all loads and stores to have a zero offset.  Furthermore, don't
 * allow add instructions to use an explicit pointer as a parameter
 * since this complicates the recognition of offsets later on, so put
 * these in a separate store.  Also, base pointers should also go into
 * a separate store, to ease dependence creation, and not be part of an
 * add or call.
 */
static void
addInstsForSimplifyingAnalysis(ir_method_t *method) {
    JITBOOLEAN added = JITFALSE;
    JITUINT32 i;

    /* Check all instructions. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); added ? i : ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        /* ir_item_t *param; */

        /* Find loads, stores and adds. */
        added = JITFALSE;
        switch (inst->type) {
            case IRADD:

                /* Check for pointers in the add instruction. */
                added |= moveAddPointersIntoStores(method, inst);
                break;

            case IRCALL: {

                /* Check for pointers in the call parameters. */
                ir_method_t *callee = IRMETHOD_getCalledMethod(method, inst);
                JITINT8 *fullName = IRMETHOD_getSignatureInString(callee);
                if (STRCMP(fullName, "__iob* libstd.__io_ftable_get_entry(System.Int32 fileno)") != 0) {
                    added |= moveCallPointersIntoStores(method, inst);
                }
                break;
            }

            case IRICALL:
            case IRVCALL:

                /* Check for pointers in the call parameters. */
                added |= moveCallPointersIntoStores(method, inst);
                break;

            case IRMEMCPY:
            case IRMEMCMP:
            case IRSTRINGCMP:

                /* Check for pointers in the memcpy instruction. */
                added |= movePointerParamIntoStore(method, inst, 1);
                added |= movePointerParamIntoStore(method, inst, 2);
                break;

            case IRFREE:

                /* Check for pointers in the free instruction. */
                added |= movePointerParamIntoStore(method, inst, 1);
                break;
        }
    }
}


/**
 * Get a list of call instructions from within the given method.  Returns all
 * instructions that make a call except those to native and library methods.
 * The returned list is ordered by instruction ID.
 */
static XanList *
getCallInsts(ir_method_t *method) {
    JITUINT32 i;
    XanList *callInsts;

    /* Initialisation. */
    callInsts = allocateNewList();
    assert(callInsts);

    /* Check all instructions. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);

        /* Find calls. */
        switch (inst->type) {
            case IRCALL:
            case IRVCALL:
            case IRICALL:
                xanList_append(callInsts, inst);
                break;
        }
    }
    return callInsts;
}


/**
 * Determine the type of a call to a known library method.
 */
static call_type_t
getKnownLibraryCallType(ir_method_t *callee) {
    JITINT8 *methodName = IRMETHOD_getSignatureInString(callee);
    if (methodName == NULL) {
        return LIBRARY_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.open(System.SByte* name,System.Int32 flags)") == 0) {
        return OPEN_CALL;
    } else if (STRCMP(methodName, "__iob* libstd.tmpfile()") == 0) {
        return TMPFILE_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.close(System.Int32 fd)") == 0) {
        return CLOSE_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.read(System.Int32 fd,System.Void* buf,System.UInt32 count)") == 0) {
        return READ_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.write(System.Int32 fd,System.Void* buf,System.UInt32 count)") == 0) {
        return WRITE_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.fgets(System.SByte* s,System.Int32 n,__iob* stream)") == 0) {
        return FGETS_CALL;
    } else if (STRCMP(methodName, "System.Void System.Environment.Exit(System.Int32 exitCode)") == 0) {
        return EXIT_CALL;
    } else if (STRCMP(methodName, "System.Void libstd.exit(System.Int32 status)") == 0) {
        return EXIT_CALL;
    } else if (STRCMP(methodName, "System.Void libstd.abort()") == 0) {
        return EXIT_CALL;
    } else if (STRCMP(methodName, "System.Void libstd.srand(System.UInt32 seed)") == 0) {
        return RAND_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.isupper(System.Int32 c)") == 0) {
        return ISUPPER_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.isalnum(System.Int32 c)") == 0) {
        return ISALNUM_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.isspace(System.Int32 c)") == 0) {
        return ISSPACE_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.isalpha(System.Int32 c)") == 0) {
        return ISALPHA_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.tolower(System.Int32 c)") == 0) {
        return TOLOWER_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.toupper(System.Int32 c)") == 0) {
        return TOUPPER_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.rand()") == 0) {
        return RAND_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fgetc(__iob* stream)") == 0) {
        return FGETC_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.getc(__iob* stream)") == 0) {
        return FGETC_CALL;
    } else if (STRCMP(methodName, "System.Int32* libstd.__errno__get_ptr()") == 0) {
        return ERRNO_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.ctime(System.UInt32* timer)") == 0) {
        return CTIME_CALL;
    } else if (STRCMP(methodName, "System.UInt32 libstd.time(System.UInt32* tptr)") == 0) {
        return TIME_CALL;
    } else if (STRCMP(methodName, "System.Double libstd.difftime(System.UInt32 time1,System.UInt32 time0)") == 0) {
        return DIFFTIME_CALL;
    } else if (STRCMP(methodName, "System.Int32 gcc4net.Crt.__maxsi3(System.Int32 a,System.Int32 b)") == 0) {
        return MAXMIN_CALL;
    } else if (STRCMP(methodName, "System.Int32 gcc4net.Crt.__minsi3(System.Int32 a,System.Int32 b)") == 0) {
        return MAXMIN_CALL;
    } else if (STRCMP(methodName, "System.UInt32 gcc4net.Crt.__uminsi3(System.UInt32 a,System.UInt32 b)") == 0) {
        return MAXMIN_CALL;
    } else if (STRCMP(methodName, "System.UInt32 gcc4net.Crt.__umaxsi3(System.UInt32 a,System.UInt32 b)") == 0) {
        return MAXMIN_CALL;
    } else if (STRCMP(methodName, "System.Int32 gcc4net.Crt.__abssi2(System.Int32 a)") == 0) {
        return ABS_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.sscanf(System.SByte* s,System.SByte* format)") == 0) {
        return SSCANF_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fscanf(__iob* stream,System.SByte* format)") == 0) {
        return FSCANF_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.sprintf(System.SByte* s,System.SByte* format)") == 0) {
        return SPRINTF_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fprintf(__iob* stream,System.SByte* format)") == 0) {
        return FPRINTF_CALL;
    } else if (STRCMP(methodName, "System.Double libstd.strtod(System.SByte* s,System.SByte** endptr)") == 0) {
        return STRTOD_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.strrchr(System.SByte* s,System.Int32 c)") == 0) {
        return STRRCHR_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.strerror(System.Int32 errnum)") == 0) {
        return STRERROR_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.strcat(System.SByte* s1,System.SByte* s2)") == 0) {
        return STRCAT_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.strcpy(System.SByte* s1,System.SByte* s2)") == 0) {
        return STRCPY_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.strncpy(System.SByte* s1,System.SByte* s2,System.UInt32 n)") == 0) {
        return STRCPY_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.strncmp(System.SByte* s1,System.SByte* s2,System.UInt32 n)") == 0) {
        return STRCMP_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.strstr(System.SByte* s1,System.SByte* s2)") == 0) {
        return STRSTR_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.strpbrk(System.SByte* s1,System.SByte* s2)") == 0) {
        return STRPBRK_CALL;
    } else if (STRCMP(methodName, "System.Void libstd.perror(System.SByte* s)") == 0) {
        return PERROR_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.printf(System.SByte* format)") == 0) {
        return PRINTF_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.puts(System.SByte* s)") == 0) {
        return PUTS_CALL;

    } else if ( (STRCMP(methodName, "System.Int32 libstd.putchar(System.Int32 c)") == 0)                    ||
                (STRCMP(methodName, "System.Void bzip2_4.BUFFER_writeUChar(System.Byte c)") == 0)           ||
                (STRCMP(methodName, "System.Void bzip2_4.BUFFER_writeBit(System.UInt32 bitToWrite)") == 0)  ||
                (STRCMP(methodName, "System.Void bzip2_4.BUFFER_writeUInt(System.UInt32 u)") == 0)          ){
        return PUTCHAR_CALL;

    } else if (STRCMP(methodName, "System.Void bzip2_4.BUFFER_needToReadNumberOfBits(System.Int32 nz,System.Void FUNCTION POINTER() customEOF)") == 0){
        return BUFFER_NEEDTOREADNUMBEROFBITS;

    } else if ( (STRCMP(methodName, "System.UInt32 bzip2_4.BUFFER_readBits(System.UInt32 n,System.Void FUNCTION POINTER() customEOF)") == 0)    ||
                (STRCMP(methodName, "System.Int32 bzip2_4.BUFFER_bsR1(System.Void FUNCTION POINTER() customEOF)") == 0)                         ){
        return BUFFER_READBITS;

    } else if (STRCMP(methodName, "System.Void bzip2_4.BUFFER_setStream(System.Int32 f,System.Byte wr)") == 0){
        return BUFFER_SETSTREAM;

    } else if (STRCMP(methodName, "System.UInt32 bzip2_4.BUFFER_getNumberOfBytesWritten()") == 0){
        return BUFFER_GETNUMBEROFBYTESWRITTEN;
    } else if (STRCMP(methodName, "System.Void bzip2_4.BUFFER_setNumberOfBytesWritten(System.UInt32 v)") == 0){
        return BUFFER_SETNUMBEROFBYTESWRITTEN;
    } else if (STRCMP(methodName, "System.Void bzip2_4.BUFFER_writeBits(System.UInt32 numBits,System.Void* memToReadFrom)") == 0){
        return BUFFER_WRITEBITS_CALL;
    } else if (STRCMP(methodName, "System.Void bzip2_4.BUFFER_writeBitsFromValue(System.UInt32 numBits,System.Int32 value)") == 0){
        return BUFFER_WRITEBITSFROMVALUE_CALL;
    } else if (STRCMP(methodName, "System.Void bzip2_4.BUFFER_flush()") == 0){
        return BUFFER_FLUSH_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.feof(__iob* stream)") == 0) {
        return FEOF_CALL;
    } else if (STRCMP(methodName, "__iob* libstd.fopen(System.SByte* filename,System.SByte* mode)") == 0) {
        return FOPEN_CALL;
    } else if (STRCMP(methodName, "System.UInt32 libstd.fread(System.Void* ptr,System.UInt32 size,System.UInt32 nmemb,__iob* stream)") == 0) {
        return FREAD_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fseek(__iob* stream,System.Int32 offset,System.Int32 whence)") == 0) {
        return FSEEK_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fclose(__iob* stream)") == 0) {
        return FCLOSE_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.remove(System.SByte* filename)") == 0) {
        return REMOVE_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fileno(__iob* stream)") == 0) {
        return FILENO_CALL;
    } else if (STRCMP(methodName, "System.UInt32 libstd.fwrite(System.Void* ptr,System.UInt32 size,System.UInt32 nmemb,__iob* stream)") == 0) {
        return FWRITE_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fputs(System.SByte* s,__iob* stream)") == 0) {
        return FPUTS_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fputc(System.Int32 c,__iob* stream)") == 0) {
        return FPUTC_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.fflush(__iob* stream)") == 0) {
        return FFLUSH_CALL;
    } else if (STRCMP(methodName, "System.Void libstd.rewind(__iob* stream)") == 0) {
        return REWIND_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.setjmp(System.Int32 buf)") == 0) {
        return SETJMP_CALL;
    } else if (STRCMP(methodName, "System.Void libstd.setbuf(__iob* stream,System.SByte* buf)") == 0) {
        return SETBUF_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.atoi(System.SByte* nptr)") == 0) {
        return ATOI_CALL;
    } else if (STRCMP(methodName, "System.Double libstd.atof(System.SByte* nptr)") == 0) {
        return ATOI_CALL;
    } else if (STRCMP(methodName, "System.Int32 libstd.atol(System.SByte* nptr)") == 0) {
        return ATOI_CALL;
    } else if (STRCMP(methodName, "System.SByte* libstd.getenv(System.SByte* name)") == 0) {
        return GETENV_CALL;
    } else if (STRCMP(methodName, "System.Void libstd.qsort(System.Void* base,System.UInt32 nmemb,System.UInt32 size,System.Int32 FUNCTION POINTER(System.Void*,System.Void*) compar)") == 0) {
        return QSORT_CALL;
    } else if (STRCMP(methodName, "System.UInt32 libstd.clock()") == 0) {
        return CLOCK_CALL;
    } else if (STRCMP(methodName, "__iob* libstd.__io_ftable_get_entry(System.Int32 fileno)") == 0) {
        return IO_FTABLE_GET_ENTRY_CALL;
    } else if (STRCMP(methodName, "System.Void* libstd.calloc(System.UInt32 nmemb,System.UInt32 size)") == 0) {
        return CALLOC_CALL;
    } else if (STRCMP(methodName, "System.Void* libstd.malloc(System.UInt32 size)") == 0) {
        return MALLOC_CALL;
    }
    //fprintf(stderr, "Call to method %s\n", IRMETHOD_getSignatureInString(callee));
    return LIBRARY_CALL;
}


/**
 * Determine the type of call being dealt with at a particular call
 * instruction.  The call either targets a known method, a generic
 * library method, or is a normal call.
 */
static call_type_t
getCallType(ir_method_t *callee) {
    if (IRPROGRAM_doesMethodBelongToALibrary(callee)) {
        if (!useKnownCalls) {
            return LIBRARY_CALL;
        }
        return getKnownLibraryCallType(callee);

    } 
    if (useKnownCalls) {
        call_type_t c;
        c   = getKnownLibraryCallType(callee);
        if (c != LIBRARY_CALL) {
            return c;
        }
    }

    return NORMAL_CALL;
}


/**
 * Determine the type of call being dealt with at a particular call
 * instruction.  The call either targets a known method, a generic
 * library method, or is a normal call.  This version of the getCallType()
 * function is for when initialising uivs, when we actually need to know
 * when a memory allocation function is called.
 */
static call_type_t
getInitialisingCallType(ir_method_t *callee) {
    if (IRPROGRAM_doesMethodBelongToALibrary(callee)) {
        call_type_t type = getKnownLibraryCallType(callee);
        if (!useKnownCalls) {
            switch (type) {
                case CALLOC_CALL:
                case MALLOC_CALL:
                case IO_FTABLE_GET_ENTRY_CALL:
                    return type;
                default:
                    return LIBRARY_CALL;
            }
        }
        return type;
    }
    return NORMAL_CALL;
}


/**
 * Get a mapping between call instructions and methods that they can call
 * within the given method.  If the constant calculateFunctionPointers
 * is JITTRUE then this only considers direct calls, since indirect calls
 * are added as the analysis phase goes on.  Otherwise, all methods that
 * can be called by each call instruction are added to the mapping.  Since
 * indirect calls can target several methods, each mapping is between a
 * call instruction and a set of call sites containing the methods.
 * Furthermore each method in the sets has to be part of the user-defined
 * program, i.e. no library calls.
 */
static SmallHashTable *
getCalledMethods(ir_method_t *method, XanList *callInsts) {
    SmallHashTable *callMap;
    XanListItem *currCall;

    /* Initialisation. */
    callMap = allocateNewHashTable();

    /* Check all instructions. */
    currCall = xanList_first(callInsts);
    while (currCall) {
        ir_instruction_t *inst = currCall->data;
        XanList *callSiteList = NULL;

        /* Create a list to store all possible callees. */
        if (inst->type != IRCALL) {
            callSiteList = allocateNewList();
            smallHashTableInsert(callMap, inst, callSiteList);
        }

        /* Only direct calls here, unless not resolving indirect calls later. */
        if (inst->type == IRCALL || !calculateFunctionPointers) {
            XanList *calleeList;
            XanListItem *calleeItem;

            /* Find the callees. */
            calleeList = IRMETHOD_getCallableMethods(inst);
            if (calculateFunctionPointers) {
                assert(calleeList && xanList_length(calleeList) == 1);
            } else {
                /* Need to order this list to ensure deterministic behaviour. */
                abort();
            }

            /* Iterate over called methods. */
            calleeItem = xanList_first(calleeList);
            while (calleeItem) {
                IR_ITEM_VALUE calleeMethodId = (IR_ITEM_VALUE)(JITNUINT)calleeItem->data;
                ir_method_t *callee = IRMETHOD_getIRMethodFromMethodID(method, calleeMethodId);
                call_site_t *callSite;

                /* Translate this called method to IR. */
                assert(callee);
                IRMETHOD_translateMethodToIR(callee);

                /* Create the new call site. */
                callSite = vllpaAllocFunction(sizeof(call_site_t), 0);
                callSite->callType = getCallType(callee);
                callSite->method = callee;

                /* Store the call site only, or save in the general set. */
                if (inst->type == IRCALL) {
                    assert(!smallHashTableContains(callMap, inst));
                    smallHashTableInsert(callMap, inst, callSite);
                } else {
                    xanList_append(callSiteList, callSite);
                }
                /* PDEBUG("Method %s calls method %s\n", IRMETHOD_getCompleteMethodName(method), IRMETHOD_getCompleteMethodName(callee)); */

                /* Next callee. */
                calleeItem = calleeItem->next;
            }

            /* Finished with the list of callees. */
            freeListUnallocated(calleeList);
        }

        /* Next call instruction. */
        currCall = currCall->next;
    }
    return callMap;
}


/**
 * Create uiv maps for each unique method called.
 */
static SmallHashTable *
createCalleeUivMaps(SmallHashTable *callMethodMap) {
    SmallHashTable *uivMaps = allocateNewHashTable();
    SmallHashTableItem *callItem = smallHashTableFirst(callMethodMap);
    while (callItem) {
        if (((ir_instruction_t *)callItem->key)->type == IRCALL) {
            call_site_t *callSite = callItem->element;
            if (!smallHashTableContains(uivMaps, callSite->method)) {
                smallHashTableInsert(uivMaps, callSite->method, allocateNewHashTable());
            }
        } else {
            XanList *callSiteList = callItem->element;
            XanListItem *callSiteItem = xanList_first(callSiteList);
            while (callSiteItem) {
                call_site_t *callSite = callSiteItem->data;
                if (!smallHashTableContains(uivMaps, callSite->method)) {
                    smallHashTableInsert(uivMaps, callSite->method, allocateNewHashTable());
                }
                callSiteItem = callSiteItem->next;
            }
        }
        callItem = smallHashTableNext(callMethodMap, callItem);
    }
    return uivMaps;
}


/**
 * Add a method from a call site to a worklist of methods to process provided
 * that it hasn't been seen already.
 */
static void
addCallSiteMethodToWorklist(call_site_t *callSite, XanList *methodWorklist, VSet *seenMethods) {
    ir_method_t *calledMethod = callSite->method;
    if (callSite->callType == NORMAL_CALL && !vSetContains(seenMethods, calledMethod)) {
        xanList_append(methodWorklist, calledMethod);
        vSetInsert(seenMethods, calledMethod);
    }
}


/**
 * Add methods to a worklist of methods to process provided that they
 * haven't been seen already.
 */
static void
addMethodsToWorklist(SmallHashTable *callMethodMap, XanList *methodWorklist, VSet *seenMethods) {
    SmallHashTableItem *currCall;

    /* Check all calls. */
    currCall = smallHashTableFirst(callMethodMap);
    while (currCall) {

        /* Direct calls only have the call site. */
        if (((ir_instruction_t *)currCall->key)->type == IRCALL) {
            addCallSiteMethodToWorklist(currCall->element, methodWorklist, seenMethods);
        } else {
            XanList *callSiteList = currCall->element;
            XanListItem *currCallSite = xanList_first(callSiteList);
            while (currCallSite) {
                call_site_t *callSite = currCallSite->data;
                addCallSiteMethodToWorklist(callSite, methodWorklist, seenMethods);
                currCallSite = currCallSite->next;
            }
        }

        /* Next call. */
        currCall = smallHashTableNext(callMethodMap, currCall);
    }
}

/**
 * Create a mapping between instructions in an SSA method back to their
 * counterparts in the original method.  This enables correct dependence
 * calculation at the end of the pass when we want to compute dependences
 * between the original method's instructions, using information about the SSA
 * method's instructions and variables.
 *
 * The SSA conversion process only inserts phi nodes and stores.  Phi nodes will
 * be inserted whenever there is a join in control flow that requires the
 * merging of two or more variables.  Therefore this can't happen in the first
 * basic block of the method.  On the other hand, stores are only added in the
 * first basic block to rename method parameters if they are redefined elsewhere
 * within the method.
 */
static SmallHashTable * createInstMappingBetweenMethods(ir_method_t *origMethod, ir_method_t *ssaMethod) {
    SmallHashTable *instMap;
    JITUINT32 origMethodNumParams;
    JITUINT32 ssaCount = 0;
    JITUINT32 b;

    /* Allocate the instruction map. */
    instMap = allocateNewHashTable();

    /* Early exit. */
    assert(IRMETHOD_getNumberOfMethodBasicBlocks(origMethod) == IRMETHOD_getNumberOfMethodBasicBlocks(ssaMethod));
    if (IRMETHOD_getNumberOfMethodBasicBlocks(origMethod) == 0) {
        return instMap;
    }

    /* Go through each basic block in the SSA method. */
    origMethodNumParams = IRMETHOD_getMethodParametersNumber(origMethod);
    for (b = 0; b < IRMETHOD_getNumberOfMethodBasicBlocks(ssaMethod); ++b) {
        IRBasicBlock *ssaBlock = IRMETHOD_getBasicBlockAtPosition(ssaMethod, b);
        IRBasicBlock *origBlock = IRMETHOD_getBasicBlockAtPosition(origMethod, b);
        ir_instruction_t *ssaInst = IRMETHOD_getFirstInstructionWithinBasicBlock(ssaMethod, ssaBlock);
        ir_instruction_t *origInst = IRMETHOD_getFirstInstructionWithinBasicBlock(origMethod, origBlock);
        JITUINT32 ssaLen = IRMETHOD_getBasicBlockLength(ssaBlock);
        JITUINT32 origLen = IRMETHOD_getBasicBlockLength(origBlock);
        JITUINT32 lenDiff = ssaLen - origLen;
        JITUINT32 i;

        /* Iterate over instructions looking for interesting ones. */
        for (i = 0; i < ssaLen; ++i) {

            /* Same block lengths. */
            if (ssaLen == origLen ||
                    /* Skip phi nodes. */
                    (b != 0 && ssaInst->type != IRPHI) ||
                    /* Skip added stores. */
                    (b == 0 && (i < origMethodNumParams || i >= origMethodNumParams + lenDiff))) {
                assert(ssaInst->type == origInst->type);

                /* Check if it's an interesting instruction, then make a mapping. */
                switch (ssaInst->type) {
                    case IRBRANCH:
                    case IRNOP:
                    case IRLABEL:
                        break;
                    default:
                        smallHashTableInsert(instMap, ssaInst, origInst);
                        break;
                }

                /* Next original instruction. */
                origInst = IRMETHOD_getNextInstruction(origMethod, origInst);
            }

            /* Next SSA instruction. */
            ssaInst = IRMETHOD_getNextInstruction(ssaMethod, ssaInst);
            ++ssaCount;
        }
    }

    /* Debugging. */
    if (ssaCount - 1 != IRMETHOD_getInstructionsNumber(ssaMethod)) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, origMethod, METHOD_PRINTER);
        IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, METHOD_PRINTER);
        fprintf(stderr, "Error setting up instruction map for %s\n", IRMETHOD_getMethodName(ssaMethod));
        fprintf(stderr, "There are %u instructions in the original method\n", IRMETHOD_getInstructionsNumber(origMethod));
        fprintf(stderr, "Analysed %u ssa instructions out of %u\n", ssaCount, IRMETHOD_getInstructionsNumber(ssaMethod));
        fprintf(stderr, "There are %u / %u basic blocks in the ssa / original methods\n", IRMETHOD_getNumberOfMethodBasicBlocks(ssaMethod), IRMETHOD_getNumberOfMethodBasicBlocks(origMethod));
        abort();
    }
    assert((ssaCount - 1) == IRMETHOD_getInstructionsNumber(ssaMethod));

    /* Return the instruction map. */
    return instMap;
}


/**
 * Print the mapping between SSA and original instructions.
 */
/* #ifdef PRINTDEBUG */
#if 0
static void
printInstMappingBetweenMethods(ir_method_t *ssaMethod, SmallHashTable *map) {
    SmallHashTableItem *item;
    PDEBUG("Instruction mapping for %s\n", IRMETHOD_getMethodName(ssaMethod));
    item = smallHashTableFirst(map);
    while (item) {
        ir_instruction_t *ssaInst = item->key;
        ir_instruction_t *origInst = item->element;
        PDEBUG("SSA inst %u is counterpart to orig inst %u\n", ssaInst->ID, origInst->ID);
        item = smallHashTableNext(map, item);
    }
}
#else
#define printInstMappingBetweenMethods(ssaMethod, map)
#endif  /* ifdef PRINTDEBUG */


/**
 * Create a set of instructions that perform field accesses within the
 * given method.  Field access instructions use IRLOADREL and IRSTOREREL
 * instructions with a constant, non-zero second parameter, whereas
 * array accesses add the offset to the base address in a separate
 * instruction.  This is, of course, reliant on the frontend producing
 * code like this, but it's what I've seen at the moment.
 */
static VSet *
createFieldAccessInstSet(ir_method_t *method) {
    VSet *fieldInsts;
    JITUINT32 i;

    /* A new set of instructions. */
    fieldInsts = allocateNewSet(3);

    /* Check all instructions. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);

        /* Check the parameters of loads and stores. */
        switch (inst->type) {
            case IRLOADREL:
            case IRSTOREREL:
                if (isNonZeroImmParam(IRMETHOD_getInstructionParameter2(inst))) {
                    vSetInsert(fieldInsts, inst);
                }
                break;
        }
    }

    /* Return the created set. */
    return fieldInsts;
}


/**
 * Check each parameter in each instruction within the given method to
 * ensure that each one has an internal type set if it has a type set.
 */
#if 0
static void
checkMethodParameterInternalTypes(ir_method_t *method) {
    JITUINT32 i, c;
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);

        /* Check parameters. */
        if (inst->param_1.type != NOPARAM) {
            assert(inst->param_1.internal_type != NOPARAM);
        }
        if (inst->param_2.type != NOPARAM) {
            assert(inst->param_2.internal_type != NOPARAM);
        }
        if (inst->param_3.type != NOPARAM) {
            assert(inst->param_3.internal_type != NOPARAM);
        }

        /* Check result. */
        if (inst->result.type != NOPARAM) {
            assert(inst->result.internal_type != NOPARAM);
        }

        /* Check call parameters. */
        for (c = 0; c < IRMETHOD_getInstructionCallParametersNumber(inst); ++c) {
            ir_item_t *param = IRMETHOD_getInstructionCallParameter(inst, c);
            if (param->type != NOPARAM) {
                assert(param->internal_type != NOPARAM);
            }
        }
    }
}
#else
#define checkMethodParameterInternalTypes(method)
#endif  /* ifdef 0 */


/**
 * Create empty sets in the given hash table that map between call
 * instructions in the given method and sets of abstract addresses read or
 * written by that call.
 */
static void
createCallReadWriteSetMappings(ir_method_t *method, SmallHashTable *callMap) {
    JITUINT32 i, numInsts = IRMETHOD_getInstructionsNumber(method);
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        abs_addr_set_t *mappedSet;
        switch (inst->type) {
            case IRCALL:
            case IRICALL:
            case IRVCALL:
            case IRLIBRARYCALL:
            case IRNATIVECALL:
                mappedSet = newAbstractAddressSetEmpty();
                smallHashTableInsert(callMap, inst, mappedSet);
                break;
        }
    }
}


/**
 * Propagate information about library calls and indirect calls between methods
 * through a single call site.
 */
static JITBOOLEAN
propagateCallSiteCallInfo(call_site_t *callSite, method_info_t *info, SmallHashTable *methodsInfo) {
    JITBOOLEAN changes = JITFALSE;

    /* Check calls that target 'normal' (i.e. non-special) methods. */
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        assert(calleeInfo);

        /* Check for library call in callee. */
        if (!info->containsLibraryCall && calleeInfo->containsLibraryCall) {
            info->containsLibraryCall = JITTRUE;
            changes = JITTRUE;
        }

        /* Check for indirect calls in callee. */
        if (!info->containsICall && calleeInfo->containsICall) {
            info->containsICall = JITTRUE;
            changes = JITTRUE;
        }
    }

    /* Mark this method if the callee is a library method. */
    else if (callSite->callType == LIBRARY_CALL && !info->containsLibraryCall) {
        info->containsLibraryCall = JITTRUE;
        changes = JITTRUE;
    }

    /* Return whether there were any changes. */
    return changes;
}


/**
 * Propagate information about library calls and indirect calls around the
 * methods in the given methods information table.  Basically, ensure that
 * if a method calls a library method or contains an indirect call, then
 * the caller method is marked as such and all callers of the method are
 * also marked as well.  This ensures that later, when computing
 * dependences between instructions, calls to methods where are library
 * call is later made are correctly identified and dealt with.  Also,
 * function call abstract addresses do not need to be propagated up to any
 * methods that do not contain indirect calls.
 */
static void
propagateCallInfo(SmallHashTable *methodsInfo) {
    JITBOOLEAN changes = JITTRUE;

    /* Guaranteed to finish because we're only setting things to JITTRUE. */
    while (changes) {
        SmallHashTableItem *currItem;
        XanListItem *callItem;

        /* Reset termination condition. */
        changes = JITFALSE;

        /* Traverse the hash table to check each method. */
        currItem = smallHashTableFirst(methodsInfo);
        while (currItem) {
            SmallHashTableItem *tableItem;
            method_info_t *info = currItem->element;

            /* Check each callee. */
            tableItem = smallHashTableFirst(info->callMethodMap);
            while (tableItem && !info->containsLibraryCall && !info->containsICall) {
                if (((ir_instruction_t *)tableItem->key)->type == IRCALL) {
                    changes |= propagateCallSiteCallInfo(tableItem->element, info, methodsInfo);
                } else {
                    XanList *callSiteList = tableItem->element;
                    XanListItem *callSiteItem = xanList_first(callSiteList);
                    while (callSiteItem && !info->containsLibraryCall && !info->containsICall) {
                        call_site_t *callSite = callSiteItem->data;
                        changes |= propagateCallSiteCallInfo(callSite, info, methodsInfo);
                        callSiteItem = callSiteItem->next;
                    }
                }
                tableItem = smallHashTableNext(info->callMethodMap, tableItem);
            }

            /* Check for an indirect call in this method. */
            callItem = xanList_first(info->callInsts);
            while (callItem && !info->containsICall) {
                ir_instruction_t *call = callItem->data;
                if (call->type == IRICALL || call->type == IRVCALL) {
                    info->containsICall = JITTRUE;
                    changes = JITTRUE;
                }
                callItem = callItem->next;
            }

            /* Next information structure. */
            currItem = smallHashTableNext(methodsInfo, currItem);
        }
    }
}


/**
 * Link a caller method to a single callee through a call site.
 */
static void
linkCallSiteCallerToCallee(call_site_t *callSite, SmallHashTable *methodsInfo) {
    /* Library calls have no info structure. */
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        assert(calleeInfo);
        calleeInfo->numCallers += 1;
    }
}


/**
 * Link caller methods to their callees through the call sites.  This
 * means increasing the callee count in each method information structure
 * and linking the call site to the version of the memory abstact address
 * map that it should use.
 */
static void
linkCallersToCallees(SmallHashTable *methodsInfo) {
    SmallHashTableItem *currItem;

    /* Traverse the hash table to check each method. */
    currItem = smallHashTableFirst(methodsInfo);
    while (currItem) {
        SmallHashTableItem *tableItem;
        method_info_t *callerInfo = currItem->element;

        /* Check each callee. */
        tableItem = smallHashTableFirst(callerInfo->callMethodMap);
        while (tableItem) {
            if (((ir_instruction_t *)tableItem->key)->type == IRCALL) {
                linkCallSiteCallerToCallee(tableItem->element, methodsInfo);
            } else {
                XanList *callSiteList = tableItem->element;
                XanListItem *callSiteItem = xanList_first(callSiteList);
                while (callSiteItem) {
                    call_site_t *callSite = callSiteItem->data;
                    linkCallSiteCallerToCallee(callSite, methodsInfo);
                    callSiteItem = callSiteItem->next;
                }
            }
            tableItem = smallHashTableNext(callerInfo->callMethodMap, tableItem);
        }

        /* Next information structure. */
        currItem = smallHashTableNext(methodsInfo, currItem);
    }
}


/**
 * Create a single method information structure.
 */
static method_info_t *
createSingleMethodInfo(ir_method_t *origMethod) {
    ir_method_t *ssaMethod;
    method_info_t *info;
    JITINT8 *origName;
    JITINT8 *cloneName;
    JITUINT32 numVars;
    JITUINT8 u;

    checkMethodParameterInternalTypes(origMethod);
#ifdef PRINTDEBUG
    PDEBUG("Setting up method info for %s\n", IRMETHOD_getCompleteMethodName(origMethod));
    IROPTIMIZER_callMethodOptimization(irOptimizer, origMethod, METHOD_PRINTER);
#endif

    /* Compute the information of the current method	*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, origMethod, BASIC_BLOCK_IDENTIFIER);

    /* Clone the method and convert to SSA form. */
    origName = IRMETHOD_getCompleteMethodName(origMethod);
    cloneName = vllpaAllocFunction((strlen((char *)origName) + 5) * sizeof(JITINT8), 1);
    vllpaWillNotFreeFunction(cloneName, (strlen((char *)origName) + 5) * sizeof(JITINT8));
    snprintf((char *)cloneName, strlen((char *)origName) + 5, "%s_ssa", (char *)origName);
    ssaMethod = IRMETHOD_cloneMethod(origMethod, cloneName);
    assert(ssaMethod && ssaMethod != origMethod);
    assert(IRMETHOD_getInstructionsNumber(ssaMethod) == IRMETHOD_getInstructionsNumber(origMethod));
    IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, SSA_CONVERTER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, ESCAPES_ANALYZER);
    assert(IRMETHOD_getMethodParametersNumber(ssaMethod) <= IRMETHOD_getNumberOfVariablesNeededByMethod(ssaMethod));
    assert(IRMETHOD_getMethodParametersNumber(ssaMethod) == IRMETHOD_getMethodParametersNumber(origMethod));
#ifdef PRINTDEBUG
    IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, METHOD_PRINTER);
#endif

    /* Add call instructions and called methods to the method information. */
    info = vllpaAllocFunction(sizeof(method_info_t), 2);
    info->ssaMethod = ssaMethod;
    info->callInsts = getCallInsts(ssaMethod);
    info->callMethodMap = getCalledMethods(ssaMethod, info->callInsts);
    info->emptyPossibleCallees = allocateNewSet(4);

    /* Field access instructions in the original method. */
    info->fieldInsts = createFieldAccessInstSet(origMethod);

    /* A mapping between ssa method and original method instructions. */
    info->instMap = createInstMappingBetweenMethods(origMethod, ssaMethod);

    /* Callee uiv maps, from call sites. */
    info->calleeUivMaps = createCalleeUivMaps(info->callMethodMap);

    /* Calculate offsets for loads and stores in explicit instructions. */
    addInstsForSimplifyingAnalysis(ssaMethod);

    /* Recompute method information because its code has been changed	*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, ESCAPES_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, LIVENESS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, BASIC_BLOCK_IDENTIFIER);

    /* Print this method's control flow graph. */
    if (ddgDumpCfg > 1) {
        char *prevDotPrinterName = getenv("DOTPRINTER_FILENAME");
        setenv("DOTPRINTER_FILENAME", "For_VLLPA", 1);
        IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, METHOD_PRINTER);
        setenv("DOTPRINTER_FILENAME", prevDotPrinterName, 1);
    }
#ifdef PRINTDEBUG
    printInstMappingBetweenMethods(ssaMethod, info->instMap);
#endif

    /* Allocate structures for uivs. */
    info->uivs = allocateNewHashTable();
    info->uivDefMap = allocateNewHashTable();
    for (u = UIV_FIELD; u <= UIV_SPECIAL; ++u) {
        VSet *uivSet = allocateNewSet(5);
        smallHashTableInsert(info->uivs, toPtr(u), uivSet);
    }

    /* And the uivs that are targets of merges. */
    info->uivMergeTargets = allocateNewHashTable();

    /* Allocate structures for abstract addresses. */
    numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(ssaMethod);
    if (numVars > 0) {
        info->varAbsAddrSets = vllpaAllocFunction(numVars * sizeof(aa_mapped_set_t *), 3);
    } else {
        info->varAbsAddrSets = NULL;
    }
    info->memAbsAddrMap = allocateNewHashTable();
    info->mergeAbsAddrMap = allocateNewHashTable();
    info->uivToMemAbsAddrKeysMap = allocateNewHashTable();
    info->funcPtrAbsAddrMap = allocateNewHashTable();
    info->calleeToCallerAbsAddrMap = allocateNewHashTable();

    /* Cached information for transferring. */
    info->cachedUivMapTransfers = allocateNewSet(6);
    info->cachedMemAbsAddrTransfers = allocateNewSet(7);

    /* Method read and write sets and instruction hash tables. */
    info->readSet = newAbstractAddressSetEmpty();
    info->writeSet = newAbstractAddressSetEmpty();
    info->callReadMap = allocateNewHashTable();
    info->callWriteMap = allocateNewHashTable();
    if (computeOutsideInsts) {
        info->readInsts = allocateNewHashTable();
        info->writeInsts = allocateNewHashTable();
    } else {
        info->readInsts = NULL;
        info->writeInsts = NULL;
    }

    /* Populate the call maps. */
    createCallReadWriteSetMappings(ssaMethod, info->callReadMap);
    createCallReadWriteSetMappings(ssaMethod, info->callWriteMap);

    /* A set for initial abstract addresses to transfer to callers. */
    info->initialAbsAddrTransferSet = newAbstractAddressSetEmpty();

    /* Finished creating this structure. */
    return info;
}


/**
 * Create new methods information structures and initialise their fields
 * for each method starting at the given method.  Add each one to the
 * given hash table.  Each method encountered is cloned and then
 * converted to SSA form (we can throw away the clone later rather than
 * converting back and possibly not getting exactly the same instructions).
 * The clone is then stored in the information structure for the original
 * method.
 */
static void
createMethodInfo(ir_method_t *method, SmallHashTable *methodsInfo) {
    XanList *methodWorklist;
    VSet *seenMethods;
    SmallHashTableItem *infoItem;

    /* Initialise each structure. */
    methodWorklist = allocateNewList();
    seenMethods = allocateNewSet(8);

    /* Add methods that have already been seen. */
    infoItem = smallHashTableFirst(methodsInfo);
    while (infoItem) {
        ir_method_t *currMethod = infoItem->key;
        if (!vSetContains(seenMethods, currMethod)) {
            vSetInsert(seenMethods, currMethod);
        }
        infoItem = smallHashTableNext(methodsInfo, infoItem);
    }

    /* Start with the current method. */
    if (!vSetContains(seenMethods, method)) {
        xanList_insert(methodWorklist, method);
        vSetInsert(seenMethods, method);
    }

    /* Populate the information hash table. */
    while (xanList_length(methodWorklist) > 0) {
        XanListItem *currItem;
        ir_method_t *currMethod;
        method_info_t *info;

        /* Get the first item from the list. */
        currItem = xanList_first(methodWorklist);
        currMethod = currItem->data;
        xanList_deleteItem(methodWorklist, currItem);

        /* Create the information structure. */
        info = createSingleMethodInfo(currMethod);

        /* Add methods called by this to the work list. */
        addMethodsToWorklist(info->callMethodMap, methodWorklist, seenMethods);

        /* Save this method information for later use. */
        smallHashTableInsert(methodsInfo, currMethod, info);
    }

    /* Clean up. */
    freeList(methodWorklist);
    freeSet(seenMethods);

    /* Propagate information about library and indirect calls. */
    propagateCallInfo(methodsInfo);

    /* Link all callers to callees. */
    linkCallersToCallees(methodsInfo);
}


/**
 * Initialise a hash table containing information about each method, the
 * control flow graph and pointer aliasing.  Each method encountered is
 * cloned and then converted to SSA form (we can throw away the clone
 * later rather than converting back and possibly not getting exactly the
 * same instructions).  The clone is then stored in the information
 * structure for the original method.
 */
static SmallHashTable * initialiseInfo(ir_method_t *method) {
    SmallHashTable *methodsInfo;

    /* Initialise the hash table. */
    methodsInfo = allocateNewHashTable();

    /* Create information structures for each method starting with this. */
    createMethodInfo(method, methodsInfo);

    /* Return the initialised information hash table. */
    return methodsInfo;
}


/**
 * Return JITTRUE if there are indirect calls in this program.
 */
static JITBOOLEAN
programHasIndirectCalls(SmallHashTable *methodsInfo) {
    SmallHashTableItem *currItem;

    /* Traverse the hash table to check each method. */
    currItem = smallHashTableFirst(methodsInfo);
    while (currItem) {
        method_info_t *info = currItem->element;
        if (info->containsICall) {
            return JITTRUE;
        }
        currItem = smallHashTableNext(methodsInfo, currItem);
    }

    /* No indirect calls found. */
    return JITFALSE;
}


/**
 * Clear a hash table of keys pointing to lists.  The keys do not get freed
 * nor the elements of the lists.  The lists themselves get freed but the
 * hash table remains intact but empty.
 */
static void
clearListHashTable(SmallHashTable *table) {
    while (smallHashTableSize(table) > 0) {
        SmallHashTableItem *tableItem = smallHashTableFirst(table);
        freeList(tableItem->element);
        smallHashTableRemove(table, tableItem->key);
    }
}


/**
 * Clear a hash table of keys pointing to sets.  The keys do not get freed
 * nor the elements of the sets.  The sets themselves get freed but the
 * hash table remains intact but empty.
 */
static void
clearSetHashTable(SmallHashTable *table) {
    while (smallHashTableSize(table) > 0) {
        SmallHashTableItem *tableItem = smallHashTableFirst(table);
        freeSet(tableItem->element);
        smallHashTableRemove(table, tableItem->key);
    }
}


/**
 * Clear a hash table of keys pointing to abstract address sets.  The keys
 * do not get freed nor the elements of the sets.  The sets themselves get
 * freed but the hash table remains intact but empty.
 */
static void
clearAbsAddrSetHashTable(SmallHashTable *table) {
    while (smallHashTableSize(table) > 0) {
        SmallHashTableItem *tableItem = smallHashTableFirst(table);
        freeAbstractAddressSet(tableItem->element);
        smallHashTableRemove(table, tableItem->key);
    }
}


/**
 * Clear mappings to sets of abstract addresses within the given hash
 * table.
 */
static void
clearAbsAddrSetMap(SmallHashTable *absAddrSetMap) {
    while (smallHashTableSize(absAddrSetMap) > 0) {
        SmallHashTableItem *tableItem = smallHashTableFirst(absAddrSetMap);
        freeAbsAddrMappedSet(tableItem->element);
        smallHashTableRemove(absAddrSetMap, tableItem->key);
    }
}


/**
 * Clear abstract address sets within an array.
 */
static void
clearAbsAddrSetArray(aa_mapped_set_t **absAddrSets, JITUINT32 length) {
    JITUINT32 i;
    for (i = 0; i < length; ++i) {
        if (absAddrSets[i]) {
            freeAbsAddrMappedSet(absAddrSets[i]);
            absAddrSets[i] = NULL;
        }
    }
}


/**
 * Clear a cache of mappings between callee abstract addresses and sets of
 * caller abstract addresses.
 */
static void
clearCalleeToCallerAbsAddrMap(SmallHashTable *calleeToCallerMap)
{
    while (smallHashTableSize(calleeToCallerMap) > 0) {
        SmallHashTableItem *tableItem = smallHashTableFirst(calleeToCallerMap);
        SmallHashTable *absAddrSetMap = tableItem->element;
        clearAbsAddrSetHashTable(absAddrSetMap);
        freeHashTable(absAddrSetMap);
        smallHashTableRemove(calleeToCallerMap, tableItem->key);
    }
}


/**
 * Clear mappings within one methods information structure between memory
 * locations, variables, unknown initial values and abstract addresses.
 */
static void
clearEphemeralInfoMappings(method_info_t *info) {
    JITUINT32 u, numVars;
    PDEBUG("Clearing info mappings for %s\n", IRMETHOD_getMethodName(info->ssaMethod));

    /* Clear uiv mappings from callees. */
    SmallHashTableItem *tableItem = smallHashTableFirst(info->calleeUivMaps);
    while (tableItem) {
        clearAbsAddrSetMap(tableItem->element);
        tableItem = smallHashTableNext(info->calleeUivMaps, tableItem);
    }

    /* Clear variable and memory sets. */
    numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(info->ssaMethod);
    if (numVars > 0) {
        clearAbsAddrSetArray(info->varAbsAddrSets, numVars);
    }
    clearAbsAddrSetMap(info->memAbsAddrMap);
    clearListHashTable(info->uivToMemAbsAddrKeysMap);

    /* Clear the set of abstract addresses to transfer to callers. */
    absAddrSetEmpty(info->initialAbsAddrTransferSet);

    /* Clear the method's read and write sets. */
    absAddrSetEmpty(info->readSet);
    absAddrSetEmpty(info->writeSet);

    /* Clear the local sets of uivs. */
    while (smallHashTableSize(info->uivs) > 0) {
        SmallHashTableItem *tableItem = smallHashTableFirst(info->uivs);
        if (tableItem->key == toPtr(UIV_FIELD)) {
            VSet *uivSet = tableItem->element;
            VSetItem *uivItem = vSetFirst(uivSet);
            while (uivItem) {
                field_uiv_holder_t *holder = getSetItemData(uivItem);
                if (holder->isList) {
                    freeList(holder->value.list);
                }
                vllpaFreeFunction(holder, sizeof(field_uiv_holder_t));
                uivItem = vSetNext(uivSet, uivItem);
            }
        }
        freeSet(tableItem->element);
        smallHashTableRemove(info->uivs, tableItem->key);
    }

    /* Clear caches. */
    vSetEmpty(info->cachedUivMapTransfers);
    vSetEmpty(info->cachedMemAbsAddrTransfers);

    /* Clear other sets and mappings. */
    smallHashTableEmpty(info->uivMergeTargets);
    clearSetHashTable(info->uivDefMap);
    clearSetHashTable(info->mergeAbsAddrMap);
    if (computeOutsideInsts) {
        clearSetHashTable(info->readInsts);
        clearSetHashTable(info->writeInsts);
    }

    /* Clear the cache of mappings from callee to caller. */
    clearCalleeToCallerAbsAddrMap(info->calleeToCallerAbsAddrMap);

    /* Set up the uiv sets again. */
    for (u = UIV_FIELD; u <= UIV_SPECIAL; ++u) {
        VSet *uivSet = allocateNewSet(9);
        smallHashTableInsert(info->uivs, toPtr(u), uivSet);
    }
}


/**
 * Clear mappings within one methods information structure that persist
 * across analysis of all methods.
 */
static void
clearPersistentInfoMappings(method_info_t *info) {
    /* Clear function pointer maps. */
    while (smallHashTableSize(info->funcPtrAbsAddrMap) > 0) {
        SmallHashTableItem *tableItem = smallHashTableFirst(info->funcPtrAbsAddrMap);
        ir_instruction_t *inst = tableItem->key;
        SmallHashTable *absAddrMap = tableItem->element;
        if (IRMETHOD_doesInstructionBelongToMethod(info->ssaMethod, inst)) {
            SmallHashTableItem *currResolveSet = smallHashTableFirst(absAddrMap);
            while (currResolveSet) {
                func_ptr_resolve_set_t *resolveSet = currResolveSet->element;
                freeAbstractAddressSet(resolveSet->funcPtrSet);
                vllpaFreeFunction(resolveSet, sizeof(func_ptr_resolve_set_t));
                currResolveSet = smallHashTableNext(absAddrMap, currResolveSet);
            }
        }
        freeHashTable(absAddrMap);
        smallHashTableRemove(info->funcPtrAbsAddrMap, inst);
    }
}


/**
 * Clear mappings within the methods information hash table between memory
 * locations, variables, unknown initial values and abstract addresses.
 * This can be called after function pointers have been resolved to reset
 * everything for the next iteration.
 */
static void
clearAllInfoMappings(SmallHashTable *methodsInfo) {
    SmallHashTableItem *currItem;

    /* Traverse the hash table to check each method. */
    currItem = smallHashTableFirst(methodsInfo);
    while (currItem) {
        method_info_t *info = currItem->element;
        clearEphemeralInfoMappings(info);
        clearPersistentInfoMappings(info);
        currItem = smallHashTableNext(methodsInfo, currItem);
    }
}


/**
 * Free a hash table of keys pointing to lists.  The keys do not get freed,
 * nor the elements of the lists.  The lists themselves get freed and the
 * hash table does too.
 */
static void
freeListHashTable(SmallHashTable *table) {
    clearListHashTable(table);
    freeHashTable(table);
}


/**
 * Free a hash table of keys pointing to sets.  The keys do not get freed,
 * nor the elements of the sets.  The sets themselves get freed and the
 * hash table does too.
 */
static void
freeSetHashTable(SmallHashTable *table) {
    clearSetHashTable(table);
    freeHashTable(table);
}


/**
 * Free a hash table of keys pointing to abstract address sets.  The keys do
 * not get freed, nor the abstract addresses.  The sets themselves get freed
 * and the hash table does too.
 */
static void
freeAbsAddrSetHashTable(SmallHashTable *table) {
    clearAbsAddrSetHashTable(table);
    freeHashTable(table);
}


/**
 * Free a call site.
 */
static void
freeCallSite(call_site_t *callSite) {
    vllpaFreeFunction(callSite, sizeof(call_site_t));
}


/**
 * Clean up memory used by the methods information hash table.  Most maps
 * will already be empty from calls to clearInfo methods, but some still
 * have useful data in.
 */
static void
freeInfo(SmallHashTable *methodsInfo) {
    SmallHashTableItem *currItem;
    SmallHashTableItem *tableItem;

    /* Traverse the hash table to free everything. */
    currItem = smallHashTableFirst(methodsInfo);
    while (currItem) {
        method_info_t *info = currItem->element;
        JITUINT32 numVars;

        /* Free the mappings between instructions. */
        freeSet(info->fieldInsts);
        freeHashTable(info->instMap);

        /* Free the call list and call site table. */
        freeList(info->callInsts);
        tableItem = smallHashTableFirst(info->callMethodMap);
        while (tableItem) {
            if (((ir_instruction_t *)tableItem->key)->type == IRCALL) {
                freeCallSite(tableItem->element);
            } else {
                XanList *callSiteList = tableItem->element;
                XanListItem *callSiteItem = xanList_first(callSiteList);
                while (callSiteItem) {
                    call_site_t *callSite = callSiteItem->data;
                    freeCallSite(callSite);
                    callSiteItem = callSiteItem->next;
                }
                freeList(callSiteList);
            }
            tableItem = smallHashTableNext(info->callMethodMap, tableItem);
        }
        freeHashTable(info->callMethodMap);
        freeSet(info->emptyPossibleCallees);

        /* Free the callee uiv maps. */
        tableItem = smallHashTableFirst(info->calleeUivMaps);
        while (tableItem) {
            clearAbsAddrSetMap(tableItem->element);
            freeHashTable(tableItem->element);
            tableItem = smallHashTableNext(info->calleeUivMaps, tableItem);
        }
        freeHashTable(info->calleeUivMaps);

        /* SSA computation structures should already be freed. */
        assert(!info->tarjan);

        /* Free the local sets of uivs. */
        while (smallHashTableSize(info->uivs) > 0) {
            SmallHashTableItem *tableItem = smallHashTableFirst(info->uivs);
            if (tableItem->key == toPtr(UIV_FIELD)) {
                VSet *uivSet = tableItem->element;
                VSetItem *uivItem = vSetFirst(uivSet);
                while (uivItem) {
                    field_uiv_holder_t *holder = getSetItemData(uivItem);
                    if (holder->isList) {
                        freeList(holder->value.list);
                    }
                    vllpaFreeFunction(holder, sizeof(field_uiv_holder_t));
                    uivItem = vSetNext(uivSet, uivItem);
                }
            }
            freeSet(tableItem->element);
            smallHashTableRemove(info->uivs, tableItem->key);
        }
        freeHashTable(info->uivs);

        /* Free the uiv definer map. */
        freeSetHashTable(info->uivDefMap);

        /* Free variable and memory sets. */
        numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(info->ssaMethod);
        if (numVars > 0) {
            clearAbsAddrSetArray(info->varAbsAddrSets, numVars);
            vllpaFreeFunction(info->varAbsAddrSets, numVars * sizeof(aa_mapped_set_t *));
        }
        clearAbsAddrSetMap(info->memAbsAddrMap);
        clearListHashTable(info->uivToMemAbsAddrKeysMap);
        freeHashTable(info->memAbsAddrMap);
        freeHashTable(info->uivToMemAbsAddrKeysMap);

        /* Free the abstract address merge map. */
        freeHashTable(info->uivMergeTargets);
        freeSetHashTable(info->mergeAbsAddrMap);

        /* Free cached transfer information. */
        freeSet(info->cachedUivMapTransfers);
        freeSet(info->cachedMemAbsAddrTransfers);

        /* Free the method's read and write sets. */
        freeAbstractAddressSet(info->readSet);
        freeAbstractAddressSet(info->writeSet);

        /* Free the mappings between calls and sets of abstract addresses. */
        freeAbsAddrSetHashTable(info->callReadMap);
        freeAbsAddrSetHashTable(info->callWriteMap);

        /* Free the mappings between abstract addresses and instructions. */
        if (computeOutsideInsts) {
            freeSetHashTable(info->readInsts);
            freeSetHashTable(info->writeInsts);
        }

        /* Free the function pointer map. */
        freeHashTable(info->funcPtrAbsAddrMap);

        /* Free the cache of mappings between callees and callers. */
        clearCalleeToCallerAbsAddrMap(info->calleeToCallerAbsAddrMap);
        freeHashTable(info->calleeToCallerAbsAddrMap);

        /* Free the initial set of abstract addresses to transfer to callees. */
        freeAbstractAddressSet(info->initialAbsAddrTransferSet);

        /* Free the SSA version of the method. */
        IRMETHOD_destroyMethod(info->ssaMethod);

        /* Free the information structure. */
        vllpaFreeFunction(info, sizeof(method_info_t));

        /* Check the original method. */
        checkMethodParameterInternalTypes(currItem->key);

        /* Next information structure. */
        currItem = smallHashTableNext(methodsInfo, currItem);
    }

    /* Free the table. */
    freeHashTable(methodsInfo);
}


/**
 * Add a method to an ordered list, where the list is ordered by the name of
 * the method.  This keeps things in the same order on each run.
 */
static void
addToOrderedMethodList(XanList *list, ir_method_t *method) {
    XanListItem *item;

    /* The easy case. */
    if (xanList_length(list) == 0) {
        xanList_append(list, method);
        return;
    }

    /* Find the position to insert the method. */
    item = xanList_first(list);
    while (item) {
        ir_method_t *other = item->data;
        if (strcmp((const char *)IRMETHOD_getCompleteMethodName(method), (const char *)IRMETHOD_getCompleteMethodName(other)) <= 0) {
            xanList_insertBefore(list, item, method);
            return;
        }
        item = xanList_next(item);
    }

    /* The method name is after all others. */
    xanList_append(list, method);
}


/**
 * Forward declaration.
 */
static void getSccs(ir_method_t *method, SmallHashTable *methodsInfo, XanStack *nodeStack, XanList *allSccs, int *index);

/**
 * Helper function to compute strongly connected components by processing a
 * single call site.
 */
static void
processCallSiteForSccs(call_site_t *callSite, method_info_t *info, SmallHashTable *methodsInfo, XanStack *nodeStack, XanList *allSccs, int *index) {
    /* Ignore library calls. */
    if (callSite->callType == NORMAL_CALL) {
        ir_method_t *callee = callSite->method;
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callee);
        assert(calleeInfo);
        if (!calleeInfo->tarjan) {
            getSccs(callee, methodsInfo, nodeStack, allSccs, index);
            info->tarjan->lowlink = MIN(info->tarjan->lowlink, calleeInfo->tarjan->lowlink);
        } else if (xanStack_contains(nodeStack, callee)) {
            info->tarjan->lowlink = MIN(info->tarjan->lowlink, calleeInfo->tarjan->index);
        }
    }
}


/**
 * Recursive function to compute strongly connected components.
 */
static void
getSccs(ir_method_t *method, SmallHashTable *methodsInfo, XanStack *nodeStack, XanList *allSccs, int *index) {
    method_info_t *info;
    ir_instruction_t **callInsts;
    JITUINT32 numCalls;
    JITUINT32 callNum;

    /* Find information about this method. */
    info = smallHashTableLookup(methodsInfo, method);

    /* Assign an index. */
    assert(info);
    assert(!info->tarjan);
    info->tarjan = vllpaAllocFunction(sizeof(tarjan_scc_t), 4);
    info->tarjan->index = *index;
    info->tarjan->lowlink = *index;
    *index = *index + 1;

    /* Get all call instructions and order them. */
    numCalls = smallHashTableSize(info->callMethodMap);
    if (numCalls > 0) {
        callInsts = (ir_instruction_t **)smallHashTableToKeyArray(info->callMethodMap);
        qsort(callInsts, numCalls, sizeof(void *), compareInstructionIDs);

        /* Save onto the stack and process callees. */
        xanStack_push(nodeStack, method);
        for (callNum = 0; callNum < numCalls; ++callNum) {
            if (callInsts[callNum]->type == IRCALL) {
                call_site_t *callSite = smallHashTableLookup(info->callMethodMap, callInsts[callNum]);
                processCallSiteForSccs(callSite, info, methodsInfo, nodeStack, allSccs, index);
            } else {
                XanList *callSiteList = smallHashTableLookup(info->callMethodMap, callInsts[callNum]);
                XanListItem *currCallSite = xanList_first(callSiteList);
                while (currCallSite) {
                    call_site_t *callSite = currCallSite->data;
                    processCallSiteForSccs(callSite, info, methodsInfo, nodeStack, allSccs, index);
                    currCallSite = currCallSite->next;
                }
            }
        }

        /* Clean up. */
        freeFunction(callInsts);

        /* Check for stronly connected component. */
        if (info->tarjan->lowlink == info->tarjan->index) {
            ir_method_t *callee = NULL;
            XanList *scc = allocateNewList();

            /* Add the SCC to a new list. */
            PDEBUG("New SCC:\n");
            while (callee != method) {
                callee = xanStack_pop(nodeStack);
                addToOrderedMethodList(scc, callee);
                PDEBUG("  SCC contains %s\n", IRMETHOD_getCompleteMethodName(callee));
            }

            /* Remember this SCC. */
            xanList_append(allSccs, scc);
        }
    }

    /* No call instructions. */
    else {
        XanList *scc = allocateNewList();
        PDEBUG("New SCC:\n");
        xanList_append(scc, method);
        PDEBUG("  SCC contains %s\n", IRMETHOD_getCompleteMethodName(method));
        xanList_append(allSccs, scc);
    }
}


/**
 * Find strongly connected components using Tarjan's algorithm, starting at
 * the given method.
 */
static XanList *
getStronglyConnectedComponents(ir_method_t *method, SmallHashTable *methodsInfo) {
    int index;
    XanList *allSccs;
    XanStack *nodeStack;
    SmallHashTableItem *infoItem;

    /* Debugging output. */
    PDEBUGNL("************************************************************\n");
    PDEBUGLITE("Performing Phase 0: Finding Strongly Connected Components\n");
    /* fprintf(stderr, "Performing Phase 0: Finding Strongly Connected Components\n"); */

    /* Initialisation. */
    allSccs = allocateNewList();
    nodeStack = xanStack_new(allocFunction, freeFunction, NULL);

    /* Perform a depth-first traversal of the graph. */
    index = 0;
    getSccs(method, methodsInfo, nodeStack, allSccs, &index);

    /* Free all created tarjan structures. */
    infoItem = smallHashTableFirst(methodsInfo);
    while (infoItem) {
        method_info_t *info = infoItem->element;
        if (info->tarjan) {
            vllpaFreeFunction(info->tarjan, sizeof(tarjan_scc_t));
            info->tarjan = NULL;
        }
        infoItem = smallHashTableNext(methodsInfo, infoItem);
    }

    /* Clean up. */
    xanStack_destroyStack(nodeStack);
    return allSccs;
}


/**
 * Append the given abstract address to the set that the given uiv can
 * point to withing the given map table.  This will ensure that all
 * abstract addresses in the final set are unique locations.  Furthermore
 * the uiv itself cannot be the uiv in the abstract address unless the
 * offset is zero, as can happen in recursive function calls, because this
 * can create loops in the data structures.
 */
static void
appendToUivAbstractAddressSet(uiv_t *uiv, SmallHashTable *uivMap, abstract_addr_t *aaAppend, method_info_t *info) {
    /* Don't allow the uiv to be in the abstract address too. */
    if (uiv != aaAppend->uiv || aaAppend->offset == 0) {
        aa_mapped_set_t *existingMapping;

        /* Find the existing set, if there. */
        if((existingMapping = smallHashTableLookup(uivMap, uiv))) {
            absAddrSetAppendAbsAddr(existingMapping->aaSet, aaAppend);
        }

        /* No existing set, create one. */
        else {
            existingMapping = createAbsAddrMappedSet(1);
            absAddrSetAppendAbsAddr(existingMapping->aaSet, aaAppend);
            smallHashTableInsert(uivMap, uiv, existingMapping);
        }
    }
}


/**
 * Append the given set of abstract addresses to the set that the given
 * uiv can point to within the given map table.  This will ensure that
 * all abstract addresses in the final set are unique locations.
 * Furthermore the uiv itself cannot be the uiv in any of the abstract
 * addresses in the set to append unless the corresponding offset is zero,
 * as can happen in recursive function calls, because this can create
 * loops in the data structures.  Will not free the given set of abstract
 * addresses.
 */
static void
appendSetToUivAbstractAddressSet(uiv_t *uiv, SmallHashTable *uivMap, abs_addr_set_t *set, method_info_t *info) {
    abstract_addr_t **array;
    JITUINT32 i, num;

    /* Add each item to the existing set in turn. */
    array = absAddrSetToArray(set, &num);
    for (i = 0; i < num; ++i) {
        appendToUivAbstractAddressSet(uiv, uivMap, array[i], info);
    }
    vllpaFreeFunction(array, num * sizeof(abstract_addr_t *));
}


/**
 * Get a set of abstract addresses that are returned by a method through
 * its return instructions.  The abstract addresses are translated to the
 * caller's abstract addresses, so no extra translation needs to occur
 * afterwards.  At the same time, record abstract addresses in the callee
 * as needing to be transferred if they are used as keys into the memory
 * abstract address mapping and have a returned variable or global uiv as
 * a prefix to their uiv.
 */
static abs_addr_set_t *
getReturnedAbstractAddressSet(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    JITUINT32 i;
    abs_addr_set_t *allReturnedSet = newAbstractAddressSetEmpty();

    /* Find all return instructions within the callee. */
    for (i = 0; i< IRMETHOD_getInstructionsNumber(calleeInfo->ssaMethod); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(calleeInfo->ssaMethod, i);
        if (inst->type == IRRET) {
            ir_item_t *param = IRMETHOD_getInstructionParameter1(inst);

            /* Get the set of returned abstract addresses. */
            if (param->type == IROFFSET) {
                abs_addr_set_t *calleeSet = getVarAbstractAddressSet(param, calleeInfo, JITFALSE);
                abs_addr_set_t *currReturnedSet;
                abs_addr_set_t *keySet;
                abstract_addr_t *aaKey;
                abstract_addr_t *aaCallee;
                /* uiv_t *uiv; */

                /* Debugging. */
                PDEBUG("    Considering return instruction %u, var %lld set: ", inst->ID, param->value.v);
                printAbstractAddressSet(calleeSet);

                /* Can we avoid this step if the mapping hasn't changed? */
                currReturnedSet = mapCalleeAbsAddrSetToCallerAbsAddrSet(calleeSet, callerInfo, calleeInfo, calleeUivMap);
                absAddrSetAppendSet(allReturnedSet, currReturnedSet);
                absAddrSetMerge(allReturnedSet, callerInfo->uivMergeTargets);
                freeAbstractAddressSet(currReturnedSet);

                /* Add memory mapping abstract address keys. */
                /* uiv = newUivFromVar(IRMETHOD_getInstructionParameter1Value(inst), calleeInfo->uivs); */
                aaCallee = absAddrSetFirst(calleeSet);
                while (aaCallee) {
                    keySet = getKeyAbsAddrSetFromUivPrefix(aaCallee->uiv, calleeInfo);
                    aaKey = absAddrSetFirst(keySet);
                    while (aaKey) {
                        PDEBUG("      Looking at key ");
                        printAbstractAddress(aaKey, JITTRUE);
                        if (!absAddrSetContains(calleeInfo->initialAbsAddrTransferSet, aaKey)) {
                            absAddrSetAppendAbsAddr(calleeInfo->initialAbsAddrTransferSet, aaKey);
                            PDEBUG("      Added ");
                            printAbstractAddress(aaKey, JITFALSE);
                            PDEBUGB(" to callee initial transfer set\n");
                        }
                        aaKey = absAddrSetNext(keySet, aaKey);
                    }
                    freeAbstractAddressSet(keySet);
                    aaCallee = absAddrSetNext(calleeSet, aaCallee);
                }
            } else if (paramIsGlobalPointer(param)) {
                abort();
            }
        }
    }

    /* Return the set of caller abstract addresses. */
    return allReturnedSet;
}


/**
 * Initialise unknown initial values for each parameter.  Returns JITTRUE
 * if there were any changes to the set of unknown initial values.
 */
static void
initialiseMethodParameterUivs(method_info_t *info) {
    JITUINT32 i;

    /* Create a new uiv and abstract address for each parameter. */
    PDEBUG("Initialising parameter uivs for %s\n", IRMETHOD_getMethodName(info->ssaMethod));
    for (i = 0; i < IRMETHOD_getMethodParametersNumber(info->ssaMethod); ++i) {
        assert(i < IRMETHOD_getNumberOfVariablesNeededByMethod(info->ssaMethod));
        uiv_t *uiv = newUivFromVar(info->ssaMethod, i, info->uivs);
        abs_addr_set_t *aaSet = newAbstractAddressInNewSet(uiv, 0);
        setVarAbstractAddressSet(i, info, aaSet, JITTRUE);
    }
}


/**
 * Record created uivs in the table for the current method and create any
 * abstract addresses caused by the result of an instruction returning a
 * pointer.
 */
static void
recordInitialisedUivsAndAbstractAddresses(method_info_t *info, ir_instruction_t *inst, uiv_t *uiv, ir_item_t *result) {

    /* Create a mapping between certain uivs and instructions. */
    if (uiv && uiv->type == UIV_ALLOC) {
        VSet *uivDefSet = smallHashTableLookup(info->uivDefMap, uiv);
        if (!uivDefSet) {
            uivDefSet = allocateNewSet(10);
            smallHashTableInsert(info->uivDefMap, uiv, uivDefSet);
        }
        if (!vSetContains(uivDefSet, inst)) {
            vSetInsert(uivDefSet, inst);
            /*         fprintf(stderr, "Method %s, inst %u defines uiv ", IRMETHOD_getMethodName(info->ssaMethod), inst->ID); */
            /*         printUivEnablePrint(uiv, "", JITTRUE); */
        }
    }

    /* Add uiv and abstract address if created. */
    if (result) {
        abs_addr_set_t *aaSet = newAbstractAddressInNewSet(uiv, 0);
        setVarAbstractAddressSet(result->value.v, info, aaSet, JITTRUE);
        PDEBUG("Inst %u, var %u: ", inst->ID, result->value.v);
        printAbstractAddressSet(aaSet);
    }
}


/**
 * Initialise unknown initial values and abstract addresses for a direct call
 * instruction.  This considers where the call is to.  If it targets a library
 * then use knowledge about the types of library call to determine whether a
 * pointer is returned from the call.
 */
static void
initialiseCallUivsAndAbstractAddresses(method_info_t *info, ir_instruction_t *call) {
    uiv_t *uiv = NULL;
    ir_item_t *result = NULL;
    ir_method_t *callee = IRMETHOD_getCalledMethod(info->ssaMethod, call);
    call_type_t callType = getInitialisingCallType(callee);

    /* Find library calls that return a pointer. */
    switch (callType) {
        case CALLOC_CALL:
        case MALLOC_CALL:
            uiv = newUivFromAlloc(info->ssaMethod, call, info->uivs);
            result = IRMETHOD_getInstructionResult(call);
            break;

        case IO_FTABLE_GET_ENTRY_CALL: {
            ir_item_t *fileno = IRMETHOD_getInstructionCallParameter(call, 0);

            /* Single file stream to add. */
            if (isImmIntParam(fileno)) {
                assert(fileno->value.v >=0 && fileno->value.v <= 2);
                uiv = newUivSpecial(fileno->value.v, info->uivs);
                result = IRMETHOD_getInstructionResult(call);
            }

            /* Add all three special file streams to the set. */
            else if (isVariableParam(fileno)) {
                JITUINT32 u;
                abs_addr_set_t *aaSet = newAbstractAddressSetEmpty();
                for (u = UIV_SPECIAL_STDIN; u <= UIV_SPECIAL_STDERR; ++u) {
                    uiv = newUivSpecial(u, info->uivs);
                    absAddrSetAppendAbsAddr(aaSet, newAbstractAddress(uiv, 0));
                }
                result = IRMETHOD_getInstructionResult(call);
                setVarAbstractAddressSet(result->value.v, info, aaSet, JITTRUE);
                result = NULL;
            }
            break;
        }

        case FOPEN_CALL:
            /* Should I deal with fopen differently? */
            uiv = newUivFromAlloc(info->ssaMethod, call, info->uivs);
            result = IRMETHOD_getInstructionResult(call);
            break;

        default:
            /* Nothing special. */
            break;
    }

    /* Create a mapping between certain uivs and instructions. */
    recordInitialisedUivsAndAbstractAddresses(info, call, uiv, result);
}


/**
 * Initialise the set of unknown initial values and register abstract
 * addresses that each instruction within the given method uses.  These
 * are initialised by considering each instruction and creating unknown
 * initial values and abstract addresses depending on the instruction
 * type.  Returns JITTRUE if there were changes to any of the sets.
 */
static void
initialiseInstructionUivsAndAbstractAddresses(method_info_t *info, SmallHashTable *methodsInfo) {
    JITUINT32 i, numInsts = IRMETHOD_getInstructionsNumber(info->ssaMethod);

    /* Create new uivs and abstract addresses by instruction type. */
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        ir_item_t *result = NULL;
        uiv_t *uiv = NULL;

        /* Action depends on instruction type. */
        assert(inst);
        switch (inst->type) {
            case IRCALL:
                initialiseCallUivsAndAbstractAddresses(info, inst);
                break;

            case IRLIBRARYCALL:
            case IRNATIVECALL: {
                JITUINT32 type = IRMETHOD_getInstructionResultType(inst);
                if (type != IRVOID && type != NOPARAM) {
                    assert(type == IROFFSET);
                    uiv = newUivFromVar(info->ssaMethod, IRMETHOD_getInstructionResult(inst)->value.v, info->uivs);
                    result = IRMETHOD_getInstructionResult(inst);
                    PDEBUG("Inst %d result type is %d\n", inst->ID, IRMETHOD_getInstructionResultType(inst));
                }
                break;
            }

            case IRADD: {
                ir_item_t *param1 = IRMETHOD_getInstructionParameter1(inst);
                ir_item_t *param2 = IRMETHOD_getInstructionParameter2(inst);

                /* Constant added to symbol is a special case. */
                if (areSymbolAndImmIntParams(param1, param2)) {
                    if (param1->type == IRSYMBOL) {
                        uiv = newUivFromGlobal(param1, param2->value.v, info->uivs);
                        PDEBUG("Created new uiv ");
                        printUiv(uiv, "", JITFALSE);
                        PDEBUGB(" at instruction %d\n", inst->ID);
                    } else {
                        uiv = newUivFromGlobal(param2, param1->value.v, info->uivs);
                    }
                    result = IRMETHOD_getInstructionResult(inst);
                }

                /* Other types added. */
                else {

                    /* Check first parameter. */
                    if (paramIsGlobalPointer(param1)) {
                        newUivFromGlobalParam(param1, info->uivs);
                    }

                    /* Check second parameter. */
                    if (paramIsGlobalPointer(param2)) {
                        uiv = newUivFromGlobalParam(param2, info->uivs);
                    }
                }
                break;
            }

            case IRSUB:
            case IRMOVE:
            case IRCONV: {
                ir_item_t *param = IRMETHOD_getInstructionParameter1(inst);
                if (paramIsGlobalPointer(param)) {
                    uiv = newUivFromGlobalParam(param, info->uivs);
                    result = IRMETHOD_getInstructionResult(inst);
                }
                break;
            }

            case IRNEWOBJ:
            case IRNEWARR:
            case IRCALLOC:
            case IRALLOCA:
            case IRALLOC:
            case IRALLOCALIGN:
                uiv = newUivFromAlloc(info->ssaMethod, inst, info->uivs);
                result = IRMETHOD_getInstructionResult(inst);
                break;
        }

        /* Create a mapping between certain uivs and instructions. */
        recordInitialisedUivsAndAbstractAddresses(info, inst, uiv, result);
    }
}


/**
 * Initialise the sets of uivs and abstract addresses that a method uses.
 * New uivs and abstract addresses are created for each parameter to the
 * method as well as for each instruction parameter that is from a global.
 * At this point, calls to memory allocation are identified and new uivs
 * and abstract addresses created for them.  Returns JITTRUE if additional
 * uivs or abstract addresses created.
 */
static void
initialiseMethodUivsAndAbstractAddresses(method_info_t *info, SmallHashTable *methodsInfo) {
    /* Initialise from parameters. */
    initialiseMethodParameterUivs(info);

    /* Create new uivs and abstract addresses by instruction type. */
    initialiseInstructionUivsAndAbstractAddresses(info, methodsInfo);
}


/**
 * Check the given method's unknown initial values to ensure that there
 * are no duplicates by checking each field of each unknown initial value.
 */
#if defined(DEBUG) && defined(CHECK_UIVS)
static void
checkMethodUivs(method_info_t *info) {
    SmallHashTableItem *uivSetItem = smallHashTableFirst(info->uivs);
    static JITBOOLEAN warned = JITFALSE;
    if (!warned) {
        fprintf(stderr, "Warning: only checking integrity of each field uiv\n");
        warned = JITTRUE;
    }

    /* Iterate over each set. */
    while (uivSetItem) {
        VSet *uivSet = uivSetItem->element;
        if (uivSetItem->key == toPtr(UIV_FIELD)) {
          VSetItem *uivItem = vSetFirst(uivSet);
          while (uivItem) {
              field_uiv_holder_t *holder = getSetItemData(uivItem);
              if (holder->isList) {
                  XanListItem *uivListItem = xanList_first(holder->value.list);
                  while (uivListItem) {
                      uiv_t *uiv = uivListItem->data;
                      checkUiv(uiv);
                      uivListItem = uivListItem->next;
                  }
              } else {
                  checkUiv(holder->value.uiv);
              }
              uivItem = vSetNext(uivSet, uivItem);
          }
        } else {
            VSetItem *uivItem = vSetFirst(uivSet);

            /* Compare each element with subsequent elements. */
            while (uivItem) {
                uiv_t *uiv = getSetItemData(uivItem);
                VSetItem *checkItem = vSetNext(uivSet, uivItem);
                checkUiv(uiv);
                while (checkItem) {
                    uiv_t *check = getSetItemData(checkItem);

                    /* Perform the check. */
                    checkUiv(check);
                    assert(uiv->type == check->type);
                    assert(!checkWhetherUivsMatch(uiv, check));

                    /* Next to check. */
                    checkItem = vSetNext(uivSet, checkItem);
                }
                uivItem = vSetNext(uivSet, uivItem);
            }
        }
        uivSetItem = smallHashTableNext(info->uivs, uivSetItem);
    }
}
#else
#define checkMethodUivs(info)
#endif  /* if defined(DEBUG) && defined(CHECK_UIVS) */


/**
 * Check all uivs in all methods.
 */
#if defined(DEBUG) && defined(CHECK_UIVS)
static void
checkAllMethodsUivs(SmallHashTable *methodsInfo)
{
    SmallHashTableItem *currItem = smallHashTableFirst(methodsInfo);
    while (currItem) {
        method_info_t *info = currItem->element;
        checkMethodUivs(info);
        currItem = smallHashTableNext(methodsInfo, currItem);
    }
}
#else
#define checkAllMethodsUivs(info)
#endif  /* if defined(DEBUG) && defined(CHECK_UIVS) */


/**
 * Initialise uivs and abstract addresses within an SCC.
 */
static void
initialiseSCCUivsAndAbstractAddresses(XanList *scc, SmallHashTable *methodsInfo) {
    XanListItem *currMethod;
    PDEBUG("Initialising uivs and abstract addresses in SCC\n");

    /* Initialise all methods in the scc. */
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        initialiseMethodUivsAndAbstractAddresses(info, methodsInfo);
        currMethod = xanList_next(currMethod);
    }
}


/**
 * Print a generic merge map.
 */
#ifdef PRINTDEBUG
static void
printMethodGenericMergeMap(method_info_t *info) {
    if (enablePDebug) {
        SmallHashTable *mergeMap = info->mergeAbsAddrMap;
        SmallHashTableItem *mapItem = smallHashTableFirst(mergeMap);
        PDEBUGNL("Merge map for %s\n", IRMETHOD_getMethodName(info->ssaMethod));
        while (mapItem) {
            VSet *mergeSet = mapItem->element;
            VSetItem *mergeItem = vSetFirst(mergeSet);
            JITBOOLEAN first = JITTRUE;
            PDEBUG("");
            printUiv(mapItem->key, "", JITFALSE);
            PDEBUGB(":");
            while (mergeItem) {
                aa_merge_t *merge = getSetItemData(mergeItem);
                if (first) {
                    PDEBUGB(" (");
                    first = JITFALSE;
                } else {
                    PDEBUGB(", (");
                }
                if (merge->type == AA_MERGE_UIVS) {
                    printUiv(merge->uiv, "", JITFALSE);
                    PDEBUGB(")");
                } else {
                    printAbstractAddress(merge->to, JITFALSE);
                    PDEBUGB("%% %u)", merge->stride);
                }
                mergeItem = vSetNext(mergeSet, mergeItem);
            }
            PDEBUGB("\n");
            mapItem = smallHashTableNext(mergeMap, mapItem);
        }
    }
}
#else
#define printMethodGenericMergeMap(mergeMap)
#endif  /* ifdef PRINTDEBUG */


/**
 * Print the unknown initial values for the given method.
 */
#ifdef PRINTDEBUG
static void
printMethodUivs(method_info_t *info) {
    if (enablePDebug) {
        SmallHashTableItem *uivSetItem = smallHashTableFirst(info->uivs);
        PDEBUGNL("Uivs for %s\n", IRMETHOD_getMethodName(info->ssaMethod));
        while (uivSetItem) {
            VSet *uivSet = uivSetItem->element;
            if (uivSetItem->key == toPtr(UIV_FIELD)) {
                VSetItem *uivFieldItem = vSetFirst(uivSet);
                while (uivFieldItem) {
                    field_uiv_holder_t *holder = getSetItemData(uivFieldItem);
                    if (holder->isList) {
                        XanListItem *uivListItem = xanList_first(holder->value.list);
                        while (uivListItem) {
                            uiv_t *uivField = uivListItem->data;
                            printUiv(uivField, "DDG_VLLPA: ", JITTRUE);
                            uivListItem = xanList_next(uivListItem);
                        }
                    } else {
                        printUiv(holder->value.uiv, "DDG_VLLPA: ", JITTRUE);
                    }
                    uivFieldItem = vSetNext(uivSet, uivFieldItem);
                }
            } else {
                VSetItem *uivItem = vSetFirst(uivSet);
                while (uivItem) {
                    uiv_t *uiv = getSetItemData(uivItem);
                    printUiv(uiv, "DDG_VLLPA: ", JITTRUE);
                    uivItem = vSetNext(uivSet, uivItem);
                }
            }
            uivSetItem = smallHashTableNext(info->uivs, uivSetItem);
        }
    }
}
#else
#define printMethodUivs(info)
#endif  /* ifdef PRINTDEBUG */


/**
 * Print the memory abstract addresses used in the given method.
 */
#ifdef PRINTDEBUG
static void
printMethodMemAbstractAddresses(method_info_t *info) {
    if (enablePDebug) {
        SmallHashTable *table = info->memAbsAddrMap;
        SmallHashTableItem *mappingItem = smallHashTableFirst(table);
        XanList *sorted = allocateNewList();
        XanListItem *aaItem;

        /* Iterate over all mappings to sort them. */
        while (mappingItem) {
            aa_mapped_set_t *mappedSet = mappingItem->element;
            abs_addr_set_t *aaSet = mappedSet->aaSet;
            if (!absAddrSetIsEmpty(aaSet)) {
                addAbsAddrToOrderedList(sorted, mappingItem->key);
            }
            mappingItem = smallHashTableNext(table, mappingItem);
        }

        /* Iterate over all mappings. */
        PDEBUGNL("Memory abstract addresses for %s\n", IRMETHOD_getMethodName(info->ssaMethod));
        aaItem = xanList_first(sorted);
        while (aaItem) {
            abstract_addr_t *aaKey = aaItem->data;
            aa_mapped_set_t *mappedSet = smallHashTableLookup(table, aaKey);
            abs_addr_set_t *aaSet = mappedSet->aaSet;
            if (!absAddrSetIsEmpty(aaSet)) {
                PDEBUG("");
                printAbstractAddress(aaKey, JITFALSE);
                PDEBUGB(": ");
                printAbstractAddressSet(aaSet);
            }
            aaItem = aaItem->next;
        }

        /* No longer need the sorted list. */
        freeList(sorted);
    }
}
#else
#define printMethodMemAbstractAddresses(info)
#endif  /* ifdef PRINTDEBUG */


/**
 * Print the variable abstract addresses used in the given method.
 */
#ifdef PRINTDEBUG
static void
printMethodVarAbstractAddresses(method_info_t *info) {
    if (enablePDebug) {
        JITUINT32 varsInMethod = IRMETHOD_getNumberOfVariablesNeededByMethod(info->ssaMethod);
        JITUINT32 v;

        /* Print them in order of variable. */
        PDEBUGNL("Variable abstract addresses for %s\n", IRMETHOD_getMethodName(info->ssaMethod));
        for (v = 0; v < varsInMethod; ++v) {
            if (info->varAbsAddrSets[v]) {
                PDEBUG("%u: ", v);
                printAbstractAddressSet(info->varAbsAddrSets[v]->aaSet);
            }
        }
    }
}
#else
#define printMethodVarAbstractAddresses(info)
#endif  /* ifdef PRINTDEBUG */


/**
 * Print a table of mappings between uivs and lists of abstract addresses.
 */
#ifdef PRINTDEBUG
static void
printUivAbstractAddressMap(SmallHashTable *uivMap) {
    if (enablePDebug) {
        SmallHashTableItem *mapItem = smallHashTableFirst(uivMap);
        while (mapItem) {
            aa_mapped_set_t *aaMap = mapItem->element;
            printUiv(mapItem->key, "DDG_VLLPA: ", JITFALSE);
            PDEBUGB(": ");
            printAbstractAddressSet(aaMap->aaSet);
            mapItem = smallHashTableNext(uivMap, mapItem);
        }
    }
}
#else
#define printUivAbstractAddressMap(uivMap)
#endif  /* ifdef PRINTDEBUG */


/**
 */
static void
addLoadMemAbsAddrsToDestSet(abstract_addr_t *aaKey, abs_addr_set_t *memAbsAddrSet, abs_addr_set_t *destSet, method_info_t *info) {
    if (memAbsAddrSet) {

        /* Add a new uiv, S@o, if required. */
        if (absAddrSetIsEmpty(memAbsAddrSet)) {
            uiv_t *uivNew = newUivFromUiv(aaKey->uiv, aaKey->offset, info->uivs);
            abstract_addr_t *aaNew = newAbstractAddress(uivNew, 0);
            absAddrSetAppendAbsAddr(memAbsAddrSet, aaNew);
        }

        /* Add to the set for the destination register. */
        /* PDEBUG("  Appending memory set for "); */
        /* printAbstractAddress(aaKey, JITFALSE); */
        /* PDEBUGB(", which is "); */
        /* printAbstractAddressSet(*memAbsAddrSet); */
        absAddrSetAppendSet(destSet, memAbsAddrSet);
        absAddrSetMerge(destSet, info->uivMergeTargets);
    /* } else { */
    /*     PDEBUG("  Memory set not available for "); */
    /*     printAbstractAddress(aaKey, JITTRUE); */
    }
}


/**
 * Create points-to relations for the given load instruction within the
 * given method.  Returns whether a new arcs or elements have been added.
 */
static JITBOOLEAN
createPointsTosInMethodLoadMem(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *srcSet;
    abs_addr_set_t *destSet;
    abstract_addr_t *aa;
    ir_item_t *result;
    JITINT32 length;
    JITBOOLEAN added = JITFALSE;

    /* Should have removed offsets earlier. */
    PDEBUG("Load memory instruction at %u (var %lld)\n", inst->ID, IRMETHOD_getInstructionResult(inst)->value.v);
    result = IRMETHOD_getInstructionResult(inst);
    srcSet = getLoadStoreAccessedAbsAddrSet(inst, info);
    destSet = newAbstractAddressSetEmpty();
    length = IRDATA_getSizeOfType(result);
    PDEBUG("  Source set is: ");
    printAbstractAddressSet(srcSet);
    /* if (length < 4) { */
    /*   fprintf(stderr, "Load of length %d seen\n", length); */
    /*   abort(); */
    /* } */

    /* Check each abstract address. */
    aa = absAddrSetFirst(srcSet);
    while (aa) {
        abs_addr_set_t *memAbsAddrSet = getMemAbstractAddressSet(aa, info, JITTRUE);
        addLoadMemAbsAddrsToDestSet(aa, memAbsAddrSet, destSet, info);
        if (length > 4) {
            JITINT32 i;
            for (i = 4; i < length; i+=4) {
                abstract_addr_t *aaKey = newAbstractAddress(aa->uiv, aa->offset + i);
                memAbsAddrSet = getMemAbstractAddressSet(aaKey, info, JITFALSE);
                addLoadMemAbsAddrsToDestSet(aaKey, memAbsAddrSet, destSet, info);
            }
        }

        /* Next abstract address. */
        aa = absAddrSetNext(srcSet, aa);
    }

    /* Set the mapping for this register. */
    added = setVarAbstractAddressSet(result->value.v, info, destSet, JITFALSE);

    /* Free up the source set. */
    freeAbstractAddressSet(srcSet);

    /* Return whether arcs or elements have been added. */
    return added;
}


/**
 * Create points-to relations for the given store instruction within the
 * given method.
 */
static void
createPointsTosInMethodStoreMem(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *destSet;
    abs_addr_set_t *srcSet;
    abstract_addr_t *aa;

    /* Should have removed offsets earlier. */
    PDEBUG("Store memory instruction at %u\n", inst->ID);
    destSet = getLoadStoreAccessedAbsAddrSet(inst, info);
    srcSet = getVarAbstractAddressSet(IRMETHOD_getInstructionParameter3(inst), info, JITTRUE);

    /* Check each abstract address. */
    aa = absAddrSetFirst(destSet);
    while (aa) {

        /* Add the source register's list to this memory location. */
        appendSetToMemAbsAddrSet(aa, info, srcSet);

        /* Next abstract address. */
        aa = absAddrSetNext(destSet, aa);
    }

    /* Free up the destination set. */
    freeAbstractAddressSet(destSet);
}


/**
 * Find the instruction that defines the given register within the given
 * method.  Assumes that the method is in SSA form so that there is a
 * single defining instruction for the register.
 */
static ir_instruction_t *
findDefiningInstInMethod(ir_item_t *reg, ir_method_t *method) {
    JITUINT32 i;

    /* Early exit. */
    if (reg->type != IROFFSET) {
        return NULL;
    }

    /* Just check each instruction until found. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        if (IRMETHOD_doesInstructionDefineVariable(method, inst, reg->value.v)) {
            return inst;
        }
    }

    /* Will get here if this is a parameter to the method. */
    assert(IRMETHOD_isTheVariableAMethodParameter(method, reg->value.v));
    return NULL;
}


/**
 * Find the instruction that defines the given register within the given
 * method, ignoring stores and moves.  Assumes that the method is in SSA form
 * so that there is a single defining instruction for each register.
 */
static ir_instruction_t *
findNonCopyDefiningInstInMethod(ir_item_t *reg, ir_method_t *method) {
    ir_instruction_t *definer;
    while ((definer = findDefiningInstInMethod(reg, method))) {
        if (definer->type == IRMOVE || definer->type == IRCONV) {
            reg = IRMETHOD_getInstructionParameter1(definer);
        } else {
            break;
        }
    }
    return definer;
}


/**
 * Infer the offset for an add of a register to a pointer by attempting
 * to pattern match the definition of the non-pointer source register
 * definition.
 */
static JITNINT
inferOffsetOfDefinition(ir_instruction_t *inst, JITBOOLEAN *noOffset) {
    /* There may not be a definer. */
    if (inst) {

        /* Check for an addition. */
        if (inst->type == IRADD || inst->type == IRADDOVF) {
            ir_item_t *src1 = IRMETHOD_getInstructionParameter1(inst);
            ir_item_t *src2 = IRMETHOD_getInstructionParameter2(inst);
            assert(src1 && src2);

            /* Check for register plus constant. */
            if (src1->type == IROFFSET && src2->type != IROFFSET) {
                return src2->value.v;
            } else if (src1->type != IROFFSET && src2->type == IROFFSET) {
                return src1->value.v;
            }
        }
    }

    /* Default is offset 0. */
    *noOffset = JITTRUE;
    return 0;
}


/**
 * Infer the stride for an add of a register to a pointer by attempting
 * to pattern match the definition of the non-pointer source register
 * definition.
 */
static JITNINT
inferStrideOfDefinition(ir_instruction_t *inst, ir_method_t *method, JITBOOLEAN *noStride) {
    /* There may not be a definer. */
    if (inst) {
        ir_item_t *src1 = IRMETHOD_getInstructionParameter1(inst);
        ir_item_t *src2 = IRMETHOD_getInstructionParameter2(inst);
        assert(src1 && src2);

        /* Check the instruction type. */
        switch (inst->type) {
            case IRADD:
            case IRADDOVF:

                /* Check for register plus constant. */
                if (src1->type == IROFFSET && src2->type != IROFFSET) {
                    return inferStrideOfDefinition(findNonCopyDefiningInstInMethod(src1, method), method, noStride);
                } else if (src1->type != IROFFSET && src2->type == IROFFSET) {
                    return inferStrideOfDefinition(findNonCopyDefiningInstInMethod(src2, method), method, noStride);
                }
                break;

            case IRMUL:
            case IRMULOVF:
                if (src1->type == IROFFSET && src2->type != IROFFSET) {
                    if (IRMETHOD_hasAFloatType(src2)) {
                        return (JITNINT)src2->value.f;
                    } else {
                        return src2->value.v;
                    }
                } else if (src1->type != IROFFSET && src2->type == IROFFSET) {
                    if (IRMETHOD_hasAFloatType(src1)) {
                        return (JITNINT)src1->value.f;
                    } else {
                        return src1->value.v;
                    }
                }
                break;

            case IRSHL:
                if (src1->type == IROFFSET && src2->type != IROFFSET) {
                    assert(!IRMETHOD_hasAFloatType(src2) && src2->value.v > 0);
                    return 2 << (src2->value.v - 1);
                }
                break;
        }
    }

    /* Default is stride 1. */
    *noStride = JITTRUE;
    return 1;
}


/**
 * Set the stride for abstract addresses in memory.
 */
static void
setStrideOfMemAbsAddrs(uiv_t *uiv, JITNINT offset, JITNINT stride, method_info_t *info) {
    SmallHashTableItem 	*aaItem;
    XanListItem		*item;
    XanList		*toUpdate;

    /* Check all mappings. */
    toUpdate	= xanList_new(allocFunction, freeFunction, NULL);
    aaItem 	= smallHashTableFirst(info->memAbsAddrMap);
    while (aaItem) {
        abstract_addr_t *aa = aaItem->key;
        assert(aa);

        /* Check this abstract address should be updated. */
        if (aa->uiv == uiv && aa->offset > offset && aa->offset != WHOLE_ARRAY_OFFSET) {
            SmallHashTableItem	*aaItemCloned;
            aaItemCloned	= allocFunction(sizeof(SmallHashTableItem));
            memcpy(aaItemCloned, aaItem, sizeof(SmallHashTableItem));
            xanList_append(toUpdate, aaItemCloned);
        }

        /* Next mapping. */
        aaItem = smallHashTableNext(info->memAbsAddrMap, aaItem);
    }

    /* Update the elements.
     */
    item	= xanList_first(toUpdate);
    while (item != NULL) {
        aaItem	= item->data;
        assert(aaItem != NULL);
        abstract_addr_t *aa = aaItem->key;
        aa_mapped_set_t *aaMap = aaItem->element;
        abs_addr_set_t *aaSet = aaMap->aaSet;
        assert(aa && aaMap && aaSet);

        JITNINT newOffset = offset + (aa->offset - offset) % stride;
        abstract_addr_t *aaKey = newAbstractAddress(uiv, newOffset);

        /* Update this memory location with the other location's set. */
        appendSetToMemAbsAddrSet(aaKey, info, aaSet);

        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyListAndData(toUpdate);

    return ;
}


/**
 * Create points-to relations based on the given source register
 * parameters from an add instruction.  For these points-to relations also
 * infer the offset and stride in case it is an array access.
 */
static void
createPointsTosInMethodAddRegsParam(ir_item_t *src1, ir_item_t *src2, method_info_t *info, abs_addr_set_t **destSet) {
    abs_addr_set_t *srcSet;
    abstract_addr_t *aa;

    /* Get the first source register's list. */
    srcSet = getVarAbstractAddressSet(src1, info, JITTRUE);
    PDEBUG("  Src set %lld is: ", src1->value.v);
    printAbstractAddressSet(srcSet);

    /* If non-empty then we can update the destination list. */
    if (!absAddrSetIsEmpty(srcSet)) {
        ir_instruction_t *defSrc2 = NULL;
        ir_item_t *srcParam = src2;
        JITBOOLEAN noOffset = JITFALSE;
        JITBOOLEAN noStride = JITFALSE;
        JITNINT c, l;

        /* Find the defining instruction, skipping stores and conversions. */
        defSrc2 = findNonCopyDefiningInstInMethod(srcParam, info->ssaMethod);
        if (defSrc2) {
            PDEBUG("  Definer of second src is inst %d\n", defSrc2->ID);
        }

        /* Infer offset and stride from the definer of the second source. */
        c = inferOffsetOfDefinition(defSrc2, &noOffset);
        l = inferStrideOfDefinition(defSrc2, info->ssaMethod, &noStride);

        /* Add to the destination set. */
        aa = absAddrSetFirst(srcSet);
        while (aa) {
            if (!noOffset) {
                if (c == 0) {
                    absAddrSetAppendAbsAddr(*destSet, aa);
                } else {
                    abstract_addr_t *aaDest = newAbstractAddress(aa->uiv, aa->offset + c);
                    absAddrSetAppendAbsAddr(*destSet, aaDest);
                }
            } else {
                /* abstract_addr_t *aaDest = newAbstractAddress(aa->uiv, WHOLE_ARRAY_OFFSET); */
                abstract_addr_t *aaDest = newAbstractAddress(aa->uiv, 0);
                absAddrSetAppendAbsAddr(*destSet, aaDest);
                addUivMergeTarget(info->uivMergeTargets, aaDest->uiv, AA_MERGE_OFFSETS);
            }
            if (!noStride) {
                setStrideOfMemAbsAddrs(aa->uiv, aa->offset, l, info);
            }
            aa = absAddrSetNext(srcSet, aa);
        }
    }
}


/**
 * Create points-to relations for the given add instruction with two
 * register parameters within the given method.  Returns the new set of
 * abstract addresses created without setting the mapping from the
 * destination variable.
 */
static abs_addr_set_t *
createPointsTosInMethodAddRegs(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *destSet;
    ir_item_t *param1;
    ir_item_t *param2;

    /* A new destination set. */
    destSet = newAbstractAddressSetEmpty();

    /* The register parameters. */
    param1 = IRMETHOD_getInstructionParameter1(inst);
    param2 = IRMETHOD_getInstructionParameter2(inst);
    assert(param1 && param2);

    /* Create points-to relations based on the first param, then the second. */
    createPointsTosInMethodAddRegsParam(param1, param2, info, &destSet);
    createPointsTosInMethodAddRegsParam(param2, param1, info, &destSet);

    /* Return the new set. */
    return destSet;
}


/**
 * Create points-to relations for the given add instruction with a pointer
 * parameter in a variable and a constant parameter within the given
 * method.  Returns the new set of abstract addresses created without
 * setting the mapping from the destination variable.
 */
static abs_addr_set_t *
createPointsTosInMethodAddConst(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *srcSet;
    abs_addr_set_t *destSet;
    ir_item_t *ptrParam;
    ir_item_t *constParam;
    abstract_addr_t *aa;

    /* The parameters. */
    ptrParam = IRMETHOD_getInstructionParameter1(inst);
    constParam = IRMETHOD_getInstructionParameter2(inst);
    assert(ptrParam && constParam);

    /* Ignore constants added to symbols. */
    destSet = NULL;
    if (!areSymbolAndImmIntParams(ptrParam, constParam)) {
        assert((ptrParam->type == IROFFSET || constParam->type == IROFFSET) && ptrParam->type != constParam->type);

        /* A new destination set. */
        destSet = newAbstractAddressSetEmpty();

        /* Make param 1 the register parameter. */
        if (constParam->type == IROFFSET) {
            ir_item_t *swap = ptrParam;
            ptrParam = constParam;
            constParam = swap;
        }

        /* Add abstract addresses into the destination set. */
        srcSet = getVarAbstractAddressSet(ptrParam, info, JITTRUE);
        aa = absAddrSetFirst(srcSet);
        while (aa) {
            if (constParam->value.v == 0) {
                absAddrSetAppendAbsAddr(destSet, aa);
            } else {
                abstract_addr_t *aaDest = newAbstractAddress(aa->uiv, aa->offset + constParam->value.v);
                absAddrSetAppendAbsAddr(destSet, aaDest);
            }
            aa = absAddrSetNext(srcSet, aa);
        }
    }

    /* Return the new set. */
    return destSet;
}


/**
 * Create points-to relations for the given add instruction within the
 * given method.  To avoid loops in the variable dependence graph that
 * increment pointers constantly, the add instruction checks the old and
 * new destination sets of abstract addresses to see if there are any
 * merge points between them.  If so, it keeps the merged addresses and
 * any non-merged addresses from the new set.  Returns whether a new arcs
 * or elements have been added.
 */
static JITBOOLEAN
createPointsTosInMethodAdd(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *destSetOld;
    abs_addr_set_t *destSetNew;
    ir_item_t *result;
    JITBOOLEAN added = JITFALSE;

    /* The old destination set. */
    PDEBUG("Add instruction at %u (var %lld)\n", inst->ID, IRMETHOD_getInstructionResult(inst)->value.v);
    result = IRMETHOD_getInstructionResult(inst);
    destSetOld = getVarAbstractAddressSet(result, info, JITTRUE);

    /* The new destination set. */
    destSetNew = NULL;
    if (IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET) {
        if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
            destSetNew = createPointsTosInMethodAddRegs(inst, info);
        } else {
            destSetNew = createPointsTosInMethodAddConst(inst, info);
        }
    } else if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
        destSetNew = createPointsTosInMethodAddConst(inst, info);
    }

    /* There may be no destination set returned for certain adds. */
    if (destSetNew) {
        /* abs_addr_set_t *destSetMerged; */

        /* /\* Perform a merge on the new set. *\/ */
        /* PDEBUG("  Dest set is "); */
        /* printAbstractAddressSet(destSetNew); */
        /* absAddrSetMerge(destSetNew, info->uivMergeTargets); */
        /* PDEBUG("  After merge "); */
        /* printAbstractAddressSet(destSetNew); */

        /* /\* Check for merge points between the two sets. *\/ */
        /* PDEBUG("  Previously  "); */
        /* printAbstractAddressSet(destSetOld); */
        /* destSetMerged = absAddrSetMergeBetweenSets(destSetNew, destSetOld, info->uivMergeTargets); */
        /* PDEBUG("  Final merge "); */
        /* printAbstractAddressSet(destSetMerged); */
        /* freeAbstractAddressSet(destSetNew); */

        /* Store set for the destination register. */
        /* added = setVarAbstractAddressSet(result->value.v, info, destSetMerged); */
        added = setVarAbstractAddressSet(result->value.v, info, destSetNew, JITFALSE);
        /*     if (added) { */
        /*       PDEBUG("Inst %u uses", inst->ID); */
        /*       if (IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET) { */
        /*         PDEBUGB(" var %llu,", IRMETHOD_getInstructionParameter1Value(inst)); */
        /*       } */
        /*       if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) { */
        /*         PDEBUGB(" var %llu", IRMETHOD_getInstructionParameter2Value(inst)); */
        /*       } */
        /*       PDEBUGB("\n"); */
        /*     } */
    }

    /* Return whether arcs or elements have been added. */
    return added;
}


/**
 * Create points-to relations for the given phi instruction within the
 * given method.  Returns whether a new arcs or elements have been added.
 */
static JITBOOLEAN
createPointsTosInMethodPhi(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *destSetOld;
    abs_addr_set_t *destSetNew;
    abs_addr_set_t *destSetMerged;
    ir_item_t *result;
    JITUINT32 i;
    JITBOOLEAN added;

    /* The old destination set. */
    PDEBUG("Phi instruction at %u (var %lld)\n", inst->ID, IRMETHOD_getInstructionResult(inst)->value.v);
    result = IRMETHOD_getInstructionResult(inst);
    destSetOld = getVarAbstractAddressSet(result, info, JITTRUE);
    destSetNew = newAbstractAddressSetEmpty();

    /* Copy source registers into the destination set. */
    for (i = 0; i < IRMETHOD_getInstructionCallParametersNumber(inst); ++i) {
        ir_item_t *param = IRMETHOD_getInstructionCallParameter(inst, i);

        /* Get the source set and copy in. */
        if (param->type == IROFFSET) {
            abs_addr_set_t *srcSet = getVarAbstractAddressSet(param, info, JITTRUE);
            absAddrSetAppendSet(destSetNew, srcSet);
        }
    }

    /* /\* Perform a merge on the new set. *\/ */
    /* absAddrSetMerge(destSetNew, info->uivMergeTargets); */

    /* /\* Check for merge points between the two sets. *\/ */
    /* destSetMerged = absAddrSetMergeBetweenSets(destSetNew, destSetOld, info->uivMergeTargets); */
    /* freeAbstractAddressSet(destSetNew); */

    /* Store set for the destination register. */
    /* added = setVarAbstractAddressSet(IRMETHOD_getInstructionResultValue(inst), info, destSetMerged); */
    added = setVarAbstractAddressSet(IRMETHOD_getInstructionResultValue(inst), info, destSetNew, JITFALSE);
    /*   if (added) { */
    /*     PDEBUG("Inst %u uses", inst->ID); */
    /*     for (i = 0; i < IRMETHOD_getInstructionCallParametersNumber(inst); ++i) { */
    /*       ir_item_t *param = IRMETHOD_getInstructionCallParameter(inst, i); */
    /*       if (param->type == IROFFSET) { */
    /*         PDEBUGB(" var %llu,", param->value.v); */
    /*       } */
    /*     } */
    /*     PDEBUGB("\n"); */
    /*   } */

    /* Return whether arcs or elements have been added. */
    return added;
}


/**
 * Copy points-to relations from the given source parameter to the given
 * destination parameter.  Returns whether new relations were added.
 */
static JITBOOLEAN
copyPointsTosBetweenParams(ir_item_t *src, ir_item_t *dest, method_info_t *info) {
    JITBOOLEAN added = JITFALSE;

    /* Source and destination must be variables. */
    if (src->type == IROFFSET && dest->type == IROFFSET) {
        abs_addr_set_t *srcSet = getVarAbstractAddressSet(src, info, JITTRUE);
        abs_addr_set_t *destSet = shareAbstractAddressSet(srcSet);
        /* abs_addr_set_t *destSet = absAddrSetClone(srcSet); */
        /*     PDEBUG("For var %llu set is:", dest->value.v); */
        /*     printAbstractAddressSet(destSet); */
        added = setVarAbstractAddressSet(dest->value.v, info, destSet, JITTRUE);
    }

    /* Return whether arcs or elements have been added. */
    return added;
}


/**
 * Create points-to relations for the given convert instruction within the
 * given method.  Returns whether a new arcs or elements have been added.
 */
static JITBOOLEAN
createPointsTosInMethodConv(ir_instruction_t *inst, method_info_t *info) {
    ir_item_t *param1 = IRMETHOD_getInstructionParameter1(inst);
    ir_item_t *result = IRMETHOD_getInstructionResult(inst);
    JITBOOLEAN added;

    /* Use the helper function to do this. */
    PDEBUG("Conv instruction at %u (var %lld)\n", inst->ID, IRMETHOD_getInstructionResult(inst)->value.v);
    added = copyPointsTosBetweenParams(param1, result, info);
    /*   if (added) { */
    /*     PDEBUG("Inst %u uses var %llu\n", inst->ID, param1->value.v); */
    /*   } */
    return added;
}


/**
 * Create points-to relations for the given move instruction within the
 * given method.  Returns whether new arcs or elements have been added.
 */
static JITBOOLEAN
createPointsTosInMethodMove(ir_instruction_t *inst, method_info_t *info) {
    ir_item_t *param1 = IRMETHOD_getInstructionParameter1(inst);
    ir_item_t *result = IRMETHOD_getInstructionResult(inst);
    JITBOOLEAN added;

    /* Use the helper function to do this. */
    PDEBUG("Move instruction at %u (var %lld)\n", inst->ID, IRMETHOD_getInstructionResult(inst)->value.v);
    added = copyPointsTosBetweenParams(param1, result, info);
    /*   if (added) { */
    /*     PDEBUG("Inst %u uses var %llu\n", inst->ID, param2->value.v); */
    /*   } */
    return added;
}


/**
 * Create points-to relations for the given get_address instruction within
 * the given method.  Returns whether new arcs or elements have been added.
 */
static JITBOOLEAN
createPointsTosInMethodGetAddress(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *destSet;
    uiv_t *uiv;
    abstract_addr_t *aa;
    JITBOOLEAN added = JITFALSE;

    /* A new destination set and the variable's source set. */
    PDEBUG("Get address instruction at %u\n", inst->ID);
    assert(IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET);
    destSet = newAbstractAddressSetEmpty();

    /* This call ensures there is a mapping in the memory table. */
    getVarAbstractAddressSet(IRMETHOD_getInstructionParameter1(inst), info, JITTRUE);

    /* The destination register points to the source, in memory. */
    uiv = newUivFromVar(info->ssaMethod, IRMETHOD_getInstructionParameter1Value(inst), info->uivs);
    aa = newAbstractAddress(uiv, 0);
    absAddrSetAppendAbsAddr(destSet, aa);

    /* Set the mapping for this register. */
    added = setVarAbstractAddressSet(IRMETHOD_getInstructionResultValue(inst), info, destSet, JITTRUE);

    /* Return whether arcs or elements have been added. */
    return added;
}


/**
 * Create points-to relations for the given memcpy instruction within the
 * given method.
 */
static void
createPointsTosInMethodMemcpy(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *srcSet;
    abs_addr_set_t *destSet;
    JITUINT32 bytes = 0;
    JITBOOLEAN allMem = JITFALSE;
    ir_item_t *bytesParam;
    abstract_addr_t *aaSrc;

    /* Get the source and destination set. */
    PDEBUG("Memcpy instruction at %u\n", inst->ID);
    assert(IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET);
    srcSet = getVarAbstractAddressSet(IRMETHOD_getInstructionParameter2(inst), info, JITTRUE);
    assert(IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET);
    destSet = getVarAbstractAddressSet(IRMETHOD_getInstructionParameter1(inst), info, JITTRUE);

    /* Determine how much data to copy. */
    bytesParam = IRMETHOD_getInstructionParameter3(inst);
    if (bytesParam->type == IROFFSET) {
        allMem = JITTRUE;
        /* PDEBUGB("all memory\n"); */
    } else {
        assert(isImmIntType(bytesParam->type));
        bytes = bytesParam->value.v;
        /* PDEBUGB("%u bytes\n", bytes); */
    }

    /* Copy data pointed to by srcSet into memory pointed to by destSet. */
    aaSrc = absAddrSetFirst(srcSet);
    while (aaSrc) {
        abs_addr_set_t *srcKeySet;
        abstract_addr_t *aaDest;
        PDEBUG("Source address ");
        printAbstractAddress(aaSrc, JITTRUE);

        /* Keys into memory. */
        srcKeySet = getKeyAbsAddrSetFromUivKey(aaSrc->uiv, info);

        /* Iterate over destinations. */
        aaDest = absAddrSetFirst(destSet);
        while (aaDest) {
            abstract_addr_t *aaSrcKey;
            PDEBUG("  Destination address ");
            printAbstractAddress(aaDest, JITTRUE);

            /* Copy the required number of bytes. */
            aaSrcKey = absAddrSetFirst(srcKeySet);
            while (aaSrcKey) {
                if (allMem || (aaSrc->offset <= aaSrcKey->offset && aaSrcKey->offset < aaSrc->offset + bytes)) {
                    abs_addr_set_t *dataSet = getMemAbstractAddressSet(aaSrcKey, info, JITTRUE);
                    abstract_addr_t *aaDestKey = newAbstractAddress(aaDest->uiv, aaDest->offset + aaSrcKey->offset - aaSrc->offset);

                    /**
                     * We can get problems when more and more mappings are added
                     * to the memory map because the algorithm doesn't terminate.
                     * More and more mappings are added with the key abstract
                     * addresses having the same uiv but different offsets.  To
                     * avoid this, we check all mappings that have keys with the
                     * same uiv as the current key.  If the current target set
                     * would have been merged into any of these then we don't
                     * create a new mapping.
                     */
                    if (!doesAbsAddrSetMergeIntoMemSet(aaDestKey->uiv, dataSet, info)) {
                        appendSetToMemAbsAddrSet(aaDestKey, info, dataSet);
                    }
                }
                aaSrcKey = absAddrSetNext(srcKeySet, aaSrcKey);
            }

            /* Next destination address. */
            aaDest = absAddrSetNext(destSet, aaDest);
        }

        /* Next source address. */
        freeAbstractAddressSet(srcKeySet);
        aaSrc = absAddrSetNext(srcSet, aaSrc);
    }
}


/**
 * Create points-to relations for the given init memory instruction within the
 * given method.  Actually, this type of instruction doesn't generate any
 * further points-to relations, but it does create abstract address merges and
 * these are important for identifying aliases.  For example, if an 8 byte
 * region of memory is inited and then a later store accesses byte 4, then
 * without the merge, there is no way of specifying that the two instructions
 * alias.  Here we actually ignore the number of bytes being initialised and
 * simply create a merge to 0 for all bytes.
 */
static void
createPointsTosInMethodInitMemory(ir_instruction_t *inst, method_info_t *info) {
    ir_item_t *baseParam;
    abs_addr_set_t *aaSet;
    abstract_addr_t *aaBase;

    /* Base address being initialised. */
    PDEBUG("Init memory instruction at %u\n", inst->ID);
    baseParam = IRMETHOD_getInstructionParameter1(inst);
    assert(baseParam->type == IROFFSET);
    aaSet = getVarAbstractAddressSet(baseParam, info, JITTRUE);

    /* Create a new merge for each abstract address. */
    aaBase = absAddrSetFirst(aaSet);
    while (aaBase) {
        abstract_addr_t *aaMerge;
        if (aaBase->offset == 0) {
            aaMerge = newAbstractAddress(aaBase->uiv, aaBase->offset + 1);
        } else {
            aaMerge = newAbstractAddress(aaBase->uiv, aaBase->offset * 2);
        }
        PDEBUG("  Creating merge from ");
        printAbstractAddress(aaBase, JITFALSE);
        PDEBUGB(" to ");
        printAbstractAddress(aaMerge, JITTRUE);
        addGenericAbsAddrMergeMapping(aaMerge, aaBase, info->mergeAbsAddrMap);
        aaBase = absAddrSetNext(aaSet, aaBase);
    }
}


/**
 * Get the set of abstract addresses accessed by a single call site.
 */
static void
appendCallSiteReturnedAbsAddrs(call_site_t *callSite, abs_addr_set_t *destSet, method_info_t *info, SmallHashTable *methodsInfo) {
    /* Ignore library calls. */
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        SmallHashTable *uivMap = smallHashTableLookup(info->calleeUivMaps, callSite->method);
        abs_addr_set_t *returnedAbsAddrSet;

        /* Get the set of abstract addresses returned from this callee. */
        PDEBUG("  Call to %s\n", IRMETHOD_getCompleteMethodName(callSite->method));
        returnedAbsAddrSet = getReturnedAbstractAddressSet(info, calleeInfo, uivMap);
        PDEBUG("  Returned set is: ");
        printAbstractAddressSet(returnedAbsAddrSet);

        /* Add to the running set. */
        absAddrSetAppendSet(destSet, returnedAbsAddrSet);
        absAddrSetMerge(destSet, info->uivMergeTargets);
        freeAbstractAddressSet(returnedAbsAddrSet);
    }
}


/**
 * Create points-to relations for the given call instruction within the given
 * method.  This requires considering each method that could be called by the
 * instruction and creating a set of abstract addresses that are returned by
 * those methods via their return instructions.
 *
 * Returns JITTRUE if the abstract address set for the destination variable
 * is altered.
 */
static JITBOOLEAN
createPointsTosInMethodCall(ir_instruction_t *inst, method_info_t *info, SmallHashTable *methodsInfo) {
    void *callTarget;
    JITBOOLEAN added = JITFALSE;

    /* Check for a valid mapping. */
    callTarget = smallHashTableLookup(info->callMethodMap, inst);
    if (callTarget) {
        abs_addr_set_t *destSet = newAbstractAddressSetEmpty();
        PDEBUG("Call instruction at %u\n", inst->ID);

        /* Target depends on instruction type. */
        if (inst->type == IRCALL) {
            appendCallSiteReturnedAbsAddrs(callTarget, destSet, info, methodsInfo);
        } else {
            XanList *callSiteList = (XanList *)callTarget;
            XanListItem *currCallSite = xanList_first(callSiteList);
            PDEBUG("  There are %d callees\n", xanList_length(callSiteList));
            while (currCallSite) {
                call_site_t *callSite = currCallSite->data;
                appendCallSiteReturnedAbsAddrs(callSite, destSet, info, methodsInfo);
                currCallSite = currCallSite->next;
            }
        }

        /* Merge, then set this call's result variable to the new set. */
        absAddrSetMerge(destSet, info->uivMergeTargets);
        added = setVarAbstractAddressSet(IRMETHOD_getInstructionResultValue(inst), info, destSet, JITTRUE);
    }

    /* Return whether anything was added. */
    return added;
}


/**
 * Create points-to relations for a given method.  Analyses each instruction
 * and creates points-to relations (abstract addresses sets) for destination
 * variables and memory locations according to the type of instruction being
 * analysed.
 *
 * Returns JITTRUE if additional points-to relations were added.
 */
static JITBOOLEAN createPointsTosInMethod(method_info_t *info, SmallHashTable *methodsInfo) {
    ir_method_t *method = info->ssaMethod;
    JITBOOLEAN added = JITFALSE;
    JITUINT32 i, numVars, instructionsNumber;

    /* Copy all variable and memory abstract address sets. */
    numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(method);
    copyMappedAbsAddrSetsArray(info->varAbsAddrSets, numVars);
    copyMappedAbsAddrSetsTable(info->memAbsAddrMap);

    /* Check out each instruction. */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);
    for (i = 0; i < instructionsNumber; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        /*     PDEBUG("Instruction %u\n", i); */

        /* Action depends on instruction type. */
        switch (inst->type) {
            case IRLOADREL:
                if (createPointsTosInMethodLoadMem(inst, info)) {
                    PDEBUG("  Added at load mem inst %u\n", inst->ID);
                    added = JITTRUE;
                }
                break;

            case IRSTOREREL:
                if (IRMETHOD_getInstructionParameter3Type(inst) == IROFFSET) {
                    createPointsTosInMethodStoreMem(inst, info);
                }
                break;

            case IRADD:
            case IRADDOVF:
                if (IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET ||
                        IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
                    if (createPointsTosInMethodAdd(inst, info)) {
                        PDEBUG("  Added at add inst %u\n", inst->ID);
                        added = JITTRUE;
                    }
                }
                break;

            case IRPHI:
                if (createPointsTosInMethodPhi(inst, info)) {
                    PDEBUG("  Added at phi inst %u\n", inst->ID);
                    added = JITTRUE;
                }
                break;

            case IRCONV:
                if (createPointsTosInMethodConv(inst, info)) {
                    PDEBUG("  Added at conv inst %u\n", inst->ID);
                    added = JITTRUE;
                }
                break;

            case IRMOVE:
                if (createPointsTosInMethodMove(inst, info)) {
                    PDEBUG("  Added at move inst %u\n", inst->ID);
                    added = JITTRUE;
                }
                break;

            case IRGETADDRESS:
                if (createPointsTosInMethodGetAddress(inst, info)) {
                    PDEBUG("  Added at get address inst %u\n", inst->ID);
                    added = JITTRUE;
                }
                break;

            case IRMEMCPY:
                createPointsTosInMethodMemcpy(inst, info);
                break;

            case IRINITMEMORY:
                createPointsTosInMethodInitMemory(inst, info);
                break;

            case IRCALL:
            case IRICALL:
            case IRVCALL:
                if (IRMETHOD_getInstructionResultType(inst) == IROFFSET) {
                    if (createPointsTosInMethodCall(inst, info, methodsInfo)) {
                        PDEBUG("  Added at call inst %u\n", inst->ID);
                        added = JITTRUE;
                    }
                }
                break;
        }
    }

    /* Check whether the variable abstract address sets have changed. */
    PDEBUG("Checking var abstract address sets\n");
    mergeMappedAbsAddrSetsArray(info->varAbsAddrSets, numVars, info->uivMergeTargets);
    if (haveMappedAbsAddrSetsChangedArray(info->varAbsAddrSets, numVars)) {
        PDEBUG("Mapped variable abstract address sets changed\n");
        added = JITTRUE;
    }

    /* Check whether the memory abstract address sets have changed. */
    PDEBUG("Checking mem abstract address sets\n");
    mergeMappedAbsAddrSetsTable(info->memAbsAddrMap, info->uivMergeTargets);
    if (haveMappedAbsAddrSetsChangedTable(info->memAbsAddrMap, JITTRUE)) {
        PDEBUG("Mapped memory abstract address sets changed\n");
        added = JITTRUE;
    }

    /* Return whether some points-tos were added. */
    /*   printMethodUivs(info); */
    printMethodVarAbstractAddresses(info);
    printMethodMemAbstractAddresses(info);
    return added;
}


/**
 * Augment the mapping between a callee uiv and the caller's sets of abstract
 * addresses.
 */
static void
augmentCalleeUivMapOneUiv(uiv_t *uivField, method_info_t *callerInfo, SmallHashTable *calleeUivMap) {
    aa_mapped_set_t *callerMappedSet = smallHashTableLookup(calleeUivMap, uivField->value.field.inner);
    /*     PDEBUG("Considering uiv "); */
    /*     printUiv(uivField, "", JITTRUE); */

    /* Check all abstract addresses in the mapping from the base uiv. */
    if (callerMappedSet) {
        abs_addr_set_t *callerSet = callerMappedSet->aaSet;
        abstract_addr_t *aaCaller = absAddrSetFirst(callerSet);

        /* Check all abstract addresses in the mapping. */
        while (aaCaller) {
            abstract_addr_t *aaKey;
            abs_addr_set_t *callerMemSet;

            /* Get the caller mapping and updated key abstract addresses. */
            if (uivField->value.field.offset != 0) {
                JITNINT newOffset = aaCaller->offset + uivField->value.field.offset;
                aaKey = newAbstractAddress(aaCaller->uiv, newOffset);
            } else {
                aaKey = aaCaller;
            }

            /* Get the memory set that the new key maps to. */
            callerMemSet = getMemAbstractAddressSet(aaKey, callerInfo, JITTRUE);

            /* Add the list to the mapping for the second uiv. */
            if (!absAddrSetIsEmpty(callerMemSet)) {
                appendSetToUivAbstractAddressSet(uivField, calleeUivMap, callerMemSet, callerInfo);
            }

            /* Next abstract address. */
            aaCaller = absAddrSetNext(callerSet, aaCaller);
        }
    }
}


/**
 * Augment the mapping between the given callee method's unknown initial
 * values and the given caller's sets of abstract addresses.
 */
static void
augmentCalleeUivMapUncached(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    VSet *uivFieldSet = smallHashTableLookup(calleeInfo->uivs, toPtr(UIV_FIELD));
    VSetItem *uivFieldItem = vSetFirst(uivFieldSet);
    PDEBUGNL("Augmenting callee uiv mappings in %s from %s\n", IRMETHOD_getMethodName(callerInfo->ssaMethod), IRMETHOD_getMethodName(calleeInfo->ssaMethod));

    /* Check all field uivs in the callee. */
    while (uivFieldItem) {
        field_uiv_holder_t *holder = getSetItemData(uivFieldItem);
        if (holder->isList) {
            XanListItem *uivListItem = xanList_first(holder->value.list);
            while (uivListItem) {
                uiv_t *uivField = uivListItem->data;
                augmentCalleeUivMapOneUiv(uivField, callerInfo, calleeUivMap);
                uivListItem = xanList_next(uivListItem);
            }
        } else {
            augmentCalleeUivMapOneUiv(holder->value.uiv, callerInfo, calleeUivMap);
        }
        uivFieldItem = vSetNext(uivFieldSet, uivFieldItem);
    }
}


/**
 * Augment the mapping between the given callee method's unknown initial
 * values and the given caller's sets of abstract addresses.
 */
static void
augmentCalleeUivMap(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    if (calleeInfo->memAbsAddrsModified || !vSetContains(callerInfo->cachedUivMapTransfers, calleeUivMap)) {
        augmentCalleeUivMapUncached(callerInfo, calleeInfo, calleeUivMap);
        vSetInsert(callerInfo->cachedUivMapTransfers, calleeUivMap);
    }
}


/**
 * Build a set of abstract addresses that will be transferred from callee
 * to caller given a set of callee abstract addresses.  As a side-effect,
 * add all callee target abstract addresses that were transferred to the
 * given set, if it is non-NULL.
 */
static abs_addr_set_t *
buildCallerAbsAddrTransferSet(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap, aa_mapped_set_t *calleeMappedSet, abs_addr_set_t *usedCalleeTargetSet) {
    abs_addr_set_t *callerFinalTargetSet = newAbstractAddressSetEmpty();
    abstract_addr_t *aaCallee = absAddrSetFirst(calleeMappedSet->aaSet);
    while (aaCallee) {
        /* PDEBUGLITE("Callee target\n"); */

        /* Ignore variable abstract addresses, unless from a method parameter. */
        if (!isBaseUivFromVar(aaCallee->uiv) || IRMETHOD_isTheVariableAMethodParameter(calleeInfo->ssaMethod, getVarFromBaseUiv(aaCallee->uiv))) {
            abs_addr_set_t *callerTargetSet;
            /* PDEBUG("  Considering callee target "); */
            /* printAbstractAddress(aaCallee, JITFALSE); */
            /*           PDEBUGLITE("Not ignored\n"); */

            /**
             * This set will be the same as has been returned on a previous
             * iteration if:
             * - aaCallee is the same abstract address
             * - the mapping between aaCallee's uiv in calleeUivMap hasn't
             *   changed
             */
            callerTargetSet = mapCalleeAbsAddrToCallerAbsAddrSet(aaCallee, callerInfo, calleeInfo, calleeUivMap, JITFALSE);
            /*           PDEBUGLITE("Mapped\n"); */
            if (callerTargetSet) {
                absAddrSetAppendSet(callerFinalTargetSet, callerTargetSet);
                /* PDEBUGB(" -> "); */
                /* printAbstractAddressSet(callerTargetSet); */
                freeAbstractAddressSet(callerTargetSet);
                /*         PDEBUGLITE("Appended\n"); */

                /* Remember this abstract address has been used. */
                if (usedCalleeTargetSet) {
                    absAddrSetAppendAbsAddr(usedCalleeTargetSet, aaCallee);
                }
            }
            /* else { */
            /*   PDEBUGB(" -> No caller target set\n"); */
            /* /\* PDEBUGLITE("Not appended\n"); *\/ */
            /* } */
        }

        /* Next callee target abstract address. */
        aaCallee = absAddrSetNext(calleeMappedSet->aaSet, aaCallee);
    }
    return callerFinalTargetSet;
}


/**
 * Augment the mappings between caller abstract addresses and sets of
 * abstract addresses so that arcs in the callee's mappings are reflected
 * as arcs in the caller's mappings.
 */
#if 0
static void
augmentCallerMemAbsAddrMap(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    SmallHashTableItem *calleeMapItem;
    /*   PDEBUGLITE("Augment caller mem map\n"); */

    /* Debugging output. */
    assert(callerInfo);
    assert(calleeInfo);
    assert(calleeUivMap);
    PDEBUGNL("Augmenting caller mem mappings in %s from %s\n", IRMETHOD_getMethodName(callerInfo->ssaMethod), IRMETHOD_getMethodName(calleeInfo->ssaMethod));

    /* Iterate over all callee mappings. */
    calleeMapItem = smallHashTableFirst(calleeInfo->memAbsAddrMap);
    while (calleeMapItem) {
        abstract_addr_t *calleeKey;
        aa_mapped_set_t *calleeMappedSet;

        /* Get the structures needed from this mapping. */
        calleeKey = calleeMapItem->key;
        calleeMappedSet = calleeMapItem->element;
        assert(calleeKey && calleeMappedSet);
        /*     PDEBUG("Considering mapping "); */
        /*     printAbstractAddress(calleeKey, JITFALSE); */
        /*     PDEBUGB(": "); */
        /*     printAbstractAddressSet(calleeMappedSet->aaSet); */

        /* Ignore variable abstract addresses, unless from a method parameter. */
        if (!isBaseUivFromVar(calleeKey->uiv) || IRMETHOD_isTheVariableAMethodParameter(calleeInfo->ssaMethod, getVarFromBaseUiv(calleeKey->uiv))) {
            abs_addr_set_t *callerFinalTargetSet;
            /*       PDEBUG("Considering callee key "); */
            /*       printAbstractAddress(calleeKey, JITTRUE); */

            /* Build a set of caller targets of the mapping. */
            callerFinalTargetSet = buildCallerAbsAddrTransferSet(callerInfo, calleeInfo, calleeUivMap, calleeMappedSet, NULL);

            /* Continue only if there's at least one target. */
            /*       PDEBUGLITE("Finished callee targets\n"); */
            if (!absAddrSetIsEmpty(callerFinalTargetSet)) {
                abs_addr_set_t *callerKeySet;

                /* Perform a merge on this set. */
                absAddrSetMerge(callerFinalTargetSet, callerInfo->uivMergeTargets);
                /*         PDEBUG("  Target set is "); */
                /*         printAbstractAddressSet(callerFinalTargetSet); */

                /* We now know we need to do this mapping. */
                callerKeySet = mapCalleeAbsAddrToCallerAbsAddrSet(calleeKey, callerInfo, calleeInfo, calleeUivMap, JITFALSE);
                /*         PDEBUGLITE("Mapped callee key\n"); */
                if (callerKeySet) {
                    abstract_addr_t *callerKey;
                    /*           PDEBUG("  Caller key set is "); */
                    /*           printAbstractAddressSet(callerKeySet); */
                    /*           PDEBUGLITE("Creating mappings\n"); */

                    /* Create new mappings between each caller key and the target set. */
                    callerKey = absAddrSetFirst(callerKeySet);
                    while (callerKey) {
                        /*             PDEBUG("  Considering caller key "); */
                        /*             printAbstractAddress(callerKey, JITTRUE); */

                        /**
                         * We can get problems when more and more mappings are added
                         * to the memory map because the algorithm doesn't terminate.
                         * More and more mappings are added with the key abstract
                         * addresses having the same uiv but different offsets.  To
                         * avoid this, we check all mappings that have keys with the
                         * same uiv as the current key.  If the current target set
                         * would have been merged into any of these then we don't
                         * create a new mapping.
                         */
                        if (!doesAbsAddrSetMergeIntoMemSet(callerKey->uiv, callerFinalTargetSet, callerInfo)) {
                            appendSetToMemAbsAddrSet(callerKey, callerInfo, callerFinalTargetSet);
                            /*             } else { */
                            /*               PDEBUG("    Merges into mem set\n"); */
                        }

                        /* Next abstract address from the mapping of the callee key. */
                        callerKey = absAddrSetNext(callerKeySet, callerKey);
                    }

                    /* No longer need the caller's key set. */
                    freeAbstractAddressSet(callerKeySet);
                    /*           PDEBUGLITE("Finished creating mappings\n"); */
                }
            }

            /* The caller's target set was empty. */
            else {
                /*         PDEBUG("  Target set is empty\n"); */
                avoidedTransfer();
            }

            /* No longer need the caller's target set. */
            freeAbstractAddressSet(callerFinalTargetSet);
        }

        /**
         * If this is a recursive call the map might have been altered, which
         * could mean that the current map item is no longer valid.  To deal
         * with this, iterate to the current callee key.
         */
        if (calleeInfo == callerInfo) {
            calleeMapItem = smallHashTableFirst(calleeInfo->memAbsAddrMap);
            while (calleeMapItem->key != calleeKey) {
                calleeMapItem = smallHashTableNext(calleeInfo->memAbsAddrMap, calleeMapItem);
            }
        }

        /* Take the next map item. */
        calleeMapItem = smallHashTableNext(calleeInfo->memAbsAddrMap, calleeMapItem);
    }
}
#endif


/**
 * Augment the mappings between caller abstract addresses and sets of
 * abstract addresses so that arcs in the callee's mappings are reflected
 * as arcs in the caller's mappings.
 */
static void
augmentCallerMemAbsAddrMapUncached(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    XanList *transferList;
    abs_addr_set_t *transferredSet;
    abs_addr_set_t *usedCalleeTargetSet;
    SmallHashTable *mergedUivs;

    /* Debugging output. */
    assert(callerInfo);
    assert(calleeInfo);
    assert(calleeUivMap);
    PDEBUGNL("Augmenting caller mem mappings in %s from %s\n", IRMETHOD_getMethodName(callerInfo->ssaMethod), IRMETHOD_getMethodName(calleeInfo->ssaMethod));

    /* Keep track of all abstract addresses transferred. */
    transferredSet = newAbstractAddressSetEmpty();

    /* The initial set of abstract addresses to transfer. */
    transferList = absAddrSetToUnorderedList(calleeInfo->initialAbsAddrTransferSet);
    /* if (absAddrSetIsEmpty(setToTransfer)) { */
    /*   PDEBUG("  Caller transfer set is empty\n"); */
    /* } */

    /* A set of callee abstract addresses used each time. */
    usedCalleeTargetSet = newAbstractAddressSetEmpty();

    /* Cache uivs that have already been merged. */
    mergedUivs = allocateNewHashTable();

    /* Iterate over all abstract addresses to transfer. */
    while (xanList_length(transferList) > 0) {
        abstract_addr_t *aaCallee;

        /* Empty the set, then add in any more necessary. */
        while (xanList_length(transferList) > 0) {
            XanListItem *calleeItem = xanList_first(transferList);
            abstract_addr_t *calleeKey = calleeItem->data;
            aa_mapped_set_t *calleeMappedSet = smallHashTableLookup(calleeInfo->memAbsAddrMap, calleeKey);
            abs_addr_set_t *callerFinalTargetSet;

            /* Just a sanity check that this is actually a key abstract address. */
            assert(calleeMappedSet);

            /* Build a set of caller targets of the mapping. */
            PDEBUG("Callee mapping ");
            printAbstractAddress(calleeKey, JITFALSE);
            PDEBUGB(": ");
            printAbstractAddressSet(calleeMappedSet->aaSet);
            callerFinalTargetSet = buildCallerAbsAddrTransferSet(callerInfo, calleeInfo, calleeUivMap, calleeMappedSet, usedCalleeTargetSet);

            /* Continue only if there's at least one target. */
            if (!absAddrSetIsEmpty(callerFinalTargetSet)) {
                abs_addr_set_t *callerKeySet;

                /* Perform a merge on this set. */
                absAddrSetMerge(callerFinalTargetSet, callerInfo->uivMergeTargets);
                /* PDEBUG("  Target set is "); */
                /* printAbstractAddressSet(callerFinalTargetSet); */

                /* We now know we need to do this mapping. */
                callerKeySet = mapCalleeAbsAddrToCallerAbsAddrSet(calleeKey, callerInfo, calleeInfo, calleeUivMap, JITFALSE);
                /*       PDEBUGLITE("Mapped callee key\n"); */
                if (callerKeySet) {
                    abstract_addr_t *callerKey;
                    /* PDEBUG("  Caller key set is "); */
                    /* printAbstractAddressSet(callerKeySet); */
                    /* PDEBUGLITE("Creating mappings\n"); */

                    /* Create new mappings between each caller key and the target set. */
                    callerKey = absAddrSetFirst(callerKeySet);
                    while (callerKey) {
                        /* PDEBUG("  Considering caller key "); */
                        /* printAbstractAddress(callerKey, JITTRUE); */
                        if (isKeyAbsAddr(callerKey, callerInfo->memAbsAddrMap)) {
                            appendSetToMemAbsAddrSet(callerKey, callerInfo, callerFinalTargetSet);
                        } else {
                            SmallHashTableItem *mergedItem = smallHashTableLookupItem(mergedUivs, callerKey->uiv);

                            /**
                             * We can get problems when more and more mappings are added
                             * to the memory map because the algorithm doesn't terminate.
                             * More and more mappings are added with the key abstract
                             * addresses having the same uiv but different offsets.  To
                             * avoid this, we check all mappings that have keys with the
                             * same uiv as the current key.  If the current target set
                             * would have been merged into any of these then we don't
                             * create a new mapping.
                             */
                            if ((mergedItem && fromPtr(JITBOOLEAN, mergedItem->element) == JITFALSE) ||
                                (!mergedItem && !doesAbsAddrSetMergeIntoMemSet(callerKey->uiv, callerFinalTargetSet, callerInfo))) {
                                appendSetToMemAbsAddrSet(callerKey, callerInfo, callerFinalTargetSet);
                                smallHashTableUniqueInsert(mergedUivs, callerKey->uiv, toPtr(JITFALSE));
                            } else if (!mergedItem) {
                                smallHashTableUniqueInsert(mergedUivs, callerKey->uiv, toPtr(JITTRUE));
                            }
                        }

                        /* Next abstract address from the mapping of the callee key. */
                        callerKey = absAddrSetNext(callerKeySet, callerKey);
                    }

                    /* No longer need the caller's key set. */
                    freeAbstractAddressSet(callerKeySet);
                    /*         PDEBUGLITE("Finished creating mappings\n"); */
                }
            }

            /* The caller's target set was empty. */
            else {
                PDEBUG("  Target set is empty\n");
            }

            /* Finished with this abstract address. */
            freeAbstractAddressSet(callerFinalTargetSet);
            absAddrSetAppendAbsAddr(transferredSet, calleeKey);
            xanList_deleteItem(transferList, calleeItem);
            smallHashTableEmpty(mergedUivs);
        }

        /**
         * We've now transferred a memory abstract address keys and the sets
         * they maps to.  We now need to transfer all other mappings from the
         * callee where the key has a prefix that is one of the target abstract
         * addresses we just transferred.  We don't need to worry about other
         * mappings since they cannot be accessed from within the caller.
         */
        aaCallee = absAddrSetFirst(usedCalleeTargetSet);
        while (aaCallee) {
            abs_addr_set_t *keySet = getKeyAbsAddrSetFromUivPrefix(aaCallee->uiv, calleeInfo);
            abstract_addr_t *aaKey = absAddrSetFirst(keySet);
            while (aaKey) {
                if (!absAddrSetContains(transferredSet, aaKey) && !xanList_find(transferList, aaKey)) {
                    xanList_append(transferList, aaKey);
                }
                aaKey = absAddrSetNext(keySet, aaKey);
            }
            freeAbstractAddressSet(keySet);
            aaCallee = absAddrSetNext(usedCalleeTargetSet, aaCallee);
        }

        /* Finished with this set. */
        absAddrSetEmpty(usedCalleeTargetSet);
    }

    /* Free the memory.
     */
    freeList(transferList);
    freeHashTable(mergedUivs);
    freeAbstractAddressSet(transferredSet);
    freeAbstractAddressSet(usedCalleeTargetSet);
}


/**
 * Augment the mappings between caller abstract addresses and sets of
 * abstract addresses so that arcs in the callee's mappings are reflected
 * as arcs in the caller's mappings.
 */
static void
augmentCallerMemAbsAddrMap(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    if (calleeInfo->memAbsAddrsModified || !vSetContains(callerInfo->cachedMemAbsAddrTransfers, calleeUivMap)) {
        augmentCallerMemAbsAddrMapUncached(callerInfo, calleeInfo, calleeUivMap);
        vSetInsert(callerInfo->cachedMemAbsAddrTransfers, calleeUivMap);
    }
}


/**
 * Augment the mapping between callee unknown initial values and caller
 * sets of abstract addresses for a single callee within the given
 * method.  Returns JITTRUE if any additional arcs were added in the
 * caller's mapping.
 */
static JITBOOLEAN
augmentCallerUivMappingsFromOneCallee(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *uivMap) {
    JITBOOLEAN someAdded = JITFALSE;

    /* /\* Make a copy of the abstract address sets. *\/ */
    /* copyMappedAbsAddrSets(uivMap); */

    /* Augment the callee uiv map. */
    augmentCalleeUivMap(callerInfo, calleeInfo, uivMap);
    /* mergeMappedAbsAddrSets(uivMap, callerInfo->uivMergeTargets); */
    /* /\* printGenericMergeMap(callerInfo->mergeAbsAddrMap); *\/ */
    /* if (haveMappedAbsAddrSetsChanged(uivMap, JITFALSE)) { */
    /*     someAdded = JITTRUE; */
    /*     PDEBUG("  Uiv abstract address sets have changed\n"); */
    /* } */

    /* Return whether mappings were added. */
    return someAdded;
}


/**
 * Add arcs into the caller's mapping between abstract addresses and sets of
 * abstract addresses for a single call site within the given method.  Returns
 * JITTRUE if any additional arcs were added in the caller's mapping.
 */
static JITBOOLEAN
augmentCallerMemMappingsFromCallSite(method_info_t *callerInfo, call_site_t *callSite, SmallHashTable *methodsInfo) {
    JITBOOLEAN someAdded = JITFALSE;
    method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
    SmallHashTable *uivMap = smallHashTableLookup(callerInfo->calleeUivMaps, callSite->method);

    /* /\* Make a copy of the abstract address sets. *\/ */
    /* copyMappedAbsAddrSets(callerInfo->memAbsAddrMap); */

    /* Augment the caller's memory map. */
    augmentCallerMemAbsAddrMap(callerInfo, calleeInfo, uivMap);
    /* mergeMappedAbsAddrSets(callerInfo->memAbsAddrMap, callerInfo->uivMergeTargets); */
    /* /\* printGenericMergeMap(callerInfo->mergeAbsAddrMap); *\/ */
    /* if (haveMappedAbsAddrSetsChanged(callerInfo->memAbsAddrMap, JITTRUE)) { */
    /*     someAdded = JITTRUE; */
    /*     PDEBUG("  Memory abstract address sets have changed\n"); */
    /* } */

    /* Return whether additional mappings were made. */
    return someAdded;
}


/**
 * Augment the mapping between callee unknown initial values and caller
 * sets of abstract addresses for each callee method with the given method.
 * At the same time, add additional arcs into the caller's mapping between
 * abstract addresses and sets of abstract addresses.  Returns JITTRUE if
 * any additional arcs were added in the caller's mapping.
 */
static JITBOOLEAN
augmentCallerMappingsFromCallees(method_info_t *callerInfo, SmallHashTable *methodsInfo, XanList *scc) {
    JITBOOLEAN someAdded = JITFALSE;
    SmallHashTableItem *callItem;

    /* Augment uiv mappings for each callee. */
    callItem = smallHashTableFirst(callerInfo->calleeUivMaps);
    while (callItem) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callItem->key);
        if (calleeInfo) {
            SmallHashTable *uivMap = callItem->element;
            someAdded |= augmentCallerUivMappingsFromOneCallee(callerInfo, calleeInfo, uivMap);
        }
        callItem = smallHashTableNext(callerInfo->calleeUivMaps, callItem);
    }

    /* Augment caller mappings from each call site. */
    callItem = smallHashTableFirst(callerInfo->callMethodMap);
    while (callItem) {
        if (((ir_instruction_t *)callItem->key)->type == IRCALL) {
            call_site_t *callSite = callItem->element;
            if (callSite->callType == NORMAL_CALL) {
                someAdded |= augmentCallerMemMappingsFromCallSite(callerInfo, callItem->element, methodsInfo);
            }
        } else {
            XanList *callSiteList = callItem->element;
            XanListItem *callSiteItem = xanList_first(callSiteList);
            while (callSiteItem) {
                call_site_t *callSite = callSiteItem->data;
                if (callSite->callType == NORMAL_CALL) {
                    someAdded |= augmentCallerMemMappingsFromCallSite(callerInfo, callSite, methodsInfo);
                }
                callSiteItem = callSiteItem->next;
            }
        }
        callItem = smallHashTableNext(callerInfo->callMethodMap, callItem);
    }

    /* Return whether any arcs were added. */
    return someAdded;
}


/**
 * Extend the callee's initial transfer set with key abstract addresses based
 * on the given uiv.
 */
static void
extendCalleeInitialTransferSet(method_info_t *calleeInfo, uiv_t *uiv) {
    abs_addr_set_t *keySet = getKeyAbsAddrSetFromUivPrefix(uiv, calleeInfo);
    abstract_addr_t *aaKey = absAddrSetFirst(keySet);
    while (aaKey) {
        if (!absAddrSetContains(calleeInfo->initialAbsAddrTransferSet, aaKey)) {
            absAddrSetAppendAbsAddr(calleeInfo->initialAbsAddrTransferSet, aaKey);
            /* PDEBUG("Added "); */
            /* printAbstractAddress(aaKey, JITFALSE); */
            /* PDEBUGB(" to callee initial transfer list\n"); */
        }
        aaKey = absAddrSetNext(keySet, aaKey);
    }
    freeAbstractAddressSet(keySet);
}


/**
 * Update the initial set of abstract addresses to be transferred from this
 * method to its callers.  Basically we want to put global abstract addresses
 * in this set if they are keys into memory.  We probably also want to put in
 * any abstract addresses reachable from this set, if they are also keys into
 * memory themselves.
 **/
static JITBOOLEAN
updateAbsAddrSetToTransfer(method_info_t *info)
{
    JITBOOLEAN additions = JITFALSE;
    SmallHashTableItem *mapItem = smallHashTableFirst(info->memAbsAddrMap);
    while (mapItem) {
        abstract_addr_t *aaKey = mapItem->key;
        if (isBaseUivFromGlobal(aaKey->uiv) && !absAddrSetContains(info->initialAbsAddrTransferSet, aaKey)) {
            absAddrSetAppendAbsAddr(info->initialAbsAddrTransferSet, aaKey);
            additions = JITTRUE;
        }
        mapItem = smallHashTableNext(info->memAbsAddrMap, mapItem);
    }
    return additions;
}


/**
 * Initialise a mapping between callee uivs and caller abstract addresses
 * for global variables, which are specified by the uivType parameter.
 * This allows global and function pointer uivs.
 */
static void
initialiseCalleeUivMapWithGlobals(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap, void *uivType) {
    VSet *uivSet;
    VSetItem *uivItem;

    /* Find the set of uivs. */
    assert(uivType != toPtr(UIV_FIELD));
    uivSet = smallHashTableLookup(calleeInfo->uivs, uivType);

    /* Iterate over this set, creating new abstract addresses. */
    uivItem = vSetFirst(uivSet);
    while (uivItem) {
        uiv_t *uivCallee = getSetItemData(uivItem);
        uiv_t *uivCaller = newUivBasedOnUiv(uivCallee, callerInfo->uivs);
        abstract_addr_t *aaCaller = newAbstractAddress(uivCaller, 0);
        appendToUivAbstractAddressSet(uivCallee, calleeUivMap, aaCaller, callerInfo);

        /* /\** */
        /*  * Removed 2014/02/20.  I don't think this causes correctness issues, but */
        /*  * should be considered as a possibility if we find that some dependences */
        /*  * on global variables are missed. */
        /*  **\/ */

        /**
         * I think that I need to do this in a different place, possibly.  It
         * seems silly to do it here.  We should really just add all globals to
         * the transfer list after analysing the method with interprocedural
         * analysis.  Keep added returned abstract addresses too though.  Then
         * we need to also make sure that all field access abstract addresses
         * are also transferred if they have a global uiv as their base, don't
         * we?  Make a check that any uiv that we transfer (in the transfer
         * function) is in the map that we're adding to above.
         **/
        /* if (uivCallee->type == UIV_GLOBAL) { */
        /*   extendCalleeInitialTransferSet(calleeInfo, uivCallee); */
        /* } */
        uivItem = vSetNext(uivSet, uivItem);
    }
}


/**
 * Initialise a mapping between callee uivs and caller abstract addresses
 * for variable and field uivs that originate from parameter variables.  At
 * the same time, record abstract addresses in the callee as needing to be
 * transferred if they are used as keys into the memory abstract address
 * mapping and have a parameter uiv as a prefix to their uiv.
 */
static void
initialiseCalleeUivMapWithParameters(ir_instruction_t *callInst, method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    VSet *uivSet;
    VSetItem *uivItem;

    /* Callee uivs from variables. */
    uivSet = smallHashTableLookup(calleeInfo->uivs, toPtr(UIV_VAR));
    uivItem = vSetFirst(uivSet);
    while (uivItem) {
        uiv_t *uiv = getSetItemData(uivItem);

        /* Find uivs from parameters. */
        assert(isBaseUivFromVar(uiv));
        if (IRMETHOD_isTheVariableAMethodParameter(calleeInfo->ssaMethod, getVarFromBaseUiv(uiv))) {

            /* Variables in callee are numbered by call parameter number. */
            ir_item_t *param = IRMETHOD_getInstructionCallParameter(callInst, getVarFromBaseUiv(uiv));
            if (param->type == IROFFSET) {
                abs_addr_set_t *varSet = getVarAbstractAddressSet(param, callerInfo, JITFALSE);

                /* Add this set to the existing set for this uiv. */
                if (!absAddrSetIsEmpty(varSet)) {
                    appendSetToUivAbstractAddressSet(uiv, calleeUivMap, varSet, callerInfo);
                }

                /* Add memory mapping abstract address keys. */
                if (getLevelOfBaseUiv(uiv) == 0) {
                    extendCalleeInitialTransferSet(calleeInfo, uiv);
                }
            }
        }

        /* Next callee uiv. */
        uivItem = vSetNext(uivSet, uivItem);
    }
}


/**
 * Transfer definitions of base uivs between two methods.  If a method or one
 * of its callees defines a UIV_ALLOC uiv that is used in an abstract
 * address in the caller, then mark the call as the defining instruction
 * of that uiv in the caller.
 */
static void
transferUivDefinesBetweenMethods(ir_instruction_t *callInst, method_info_t *callerInfo, method_info_t *calleeInfo) {
    VSet *uivAllocSet = smallHashTableLookup(calleeInfo->uivs, toPtr(UIV_ALLOC));
    VSetItem *uivAllocItem = vSetFirst(uivAllocSet);

    /* Check all UIV_ALLOC uivs from the callee that are used in the caller. */
    while (uivAllocItem) {
        uiv_t *uivCallee = getSetItemData(uivAllocItem);

        /* Check this uiv exists in the caller. */
        if (uivExistsInTable(uivCallee, callerInfo->uivs)) {
            uiv_t *uivCaller = newUivBasedOnUiv(uivCallee, callerInfo->uivs);

            /* If the defining instruction is in the caller, don't add. */
            if (!IRMETHOD_doesInstructionBelongToMethod(callerInfo->ssaMethod, getInstFromBaseUiv(uivCaller))) {
                VSet *defSet = smallHashTableLookup(callerInfo->uivDefMap, uivCaller);

                /* Create a new set if there isn't one already. */
                if (!defSet) {
                    defSet = allocateNewSet(11);
                    smallHashTableInsert(callerInfo->uivDefMap, uivCaller, defSet);
                }

                /* Only add the call if it's not there already. */
                if (!vSetContains(defSet, callInst)) {
                    vSetInsert(defSet, callInst);
                    /*           fprintf(stderr, "Method %s, inst %u defines uiv ", IRMETHOD_getMethodName(callerInfo->ssaMethod), callInst->ID); */
                    /*           printUivEnablePrint(uivCaller, "", JITTRUE); */
                }
            }
        }

        /* Next UIV_ALLOC uiv. */
        uivAllocItem = vSetNext(uivAllocSet, uivAllocItem);
    }
}


/**
 * Transfer uiv defines between methods in a single call site.
 */
static void
transferUivDefinesAcrossCallSite(ir_instruction_t *callInst, call_site_t *callSite, method_info_t *callerInfo, SmallHashTable *methodsInfo) {
    /* Ignore library calls. */
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        transferUivDefinesBetweenMethods(callInst, callerInfo, calleeInfo);
    }
}


/**
 * Mark call instructions as definers of certain uivs if those uivs are used
 * within the given method and defined in a callee.
 */
static void
markUivDefinesAcrossCalls(method_info_t *callerInfo, SmallHashTable *methodsInfo) {
    SmallHashTableItem *callItem = smallHashTableFirst(callerInfo->callMethodMap);

    /* Iterate over all call sites. */
    while (callItem) {
        ir_instruction_t *callInst = callItem->key;
        if (callInst->type == IRCALL) {
            transferUivDefinesAcrossCallSite(callInst, callItem->element, callerInfo, methodsInfo);
        } else {
            XanList *callSiteList = callItem->element;
            XanListItem *currCallSite = xanList_first(callSiteList);
            while (currCallSite) {
                call_site_t *callSite = currCallSite->data;
                transferUivDefinesAcrossCallSite(callInst, callSite, callerInfo, methodsInfo);
                currCallSite = currCallSite->next;
            }
        }

        /* Next call instruction. */
        callItem = smallHashTableNext(callerInfo->callMethodMap, callItem);
    }
}


/**
 * Initialise the uiv map for one callee.
 */
static JITBOOLEAN
initialiseOneCalleeUivMap(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *uivMap) {
    JITBOOLEAN someAdded = JITFALSE;

    /* /\* Make a copy of the abstract address sets. *\/ */
    /* copyMappedAbsAddrSets(uivMap); */

    /* Initialise uiv maps for this callee (must be done before returns). */
    initialiseCalleeUivMapWithGlobals(callerInfo, calleeInfo, uivMap, toPtr(UIV_GLOBAL));
    initialiseCalleeUivMapWithGlobals(callerInfo, calleeInfo, uivMap, toPtr(UIV_FUNC));

    /* /\* Check for changes in the abstract address sets. *\/ */
    /* mergeMappedAbsAddrSets(uivMap, callerInfo->uivMergeTargets); */
    /* /\* printGenericMergeMap(callerInfo->mergeAbsAddrMap); *\/ */
    /* someAdded = haveMappedAbsAddrSetsChanged(uivMap, JITFALSE); */
    return someAdded;
}


/**
 * Initialise the uiv map between caller and callee for a particular call instruction.
 */
static JITBOOLEAN
initialiseCalleeUivMapFromCall(method_info_t *callerInfo, ir_instruction_t *callInst, call_site_t *callSite, SmallHashTable *methodsInfo) {
    JITBOOLEAN someAdded = JITFALSE;

    /* Ignore library calls. */
    if (callSite->callType == NORMAL_CALL) {
        SmallHashTable *uivMap = smallHashTableLookup(callerInfo->calleeUivMaps, callSite->method);
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);

        /* /\* Make a copy of the abstract address sets. *\/ */
        /* copyMappedAbsAddrSets(uivMap); */

        /* Initialise uiv map for this instruction (must be done before returns). */
        initialiseCalleeUivMapWithParameters(callInst, callerInfo, calleeInfo, uivMap);

        /* /\* Check for changes in the abstract address sets. *\/ */
        /* mergeMappedAbsAddrSets(uivMap, callerInfo->uivMergeTargets); */
        /* /\* printGenericMergeMap(callerInfo->mergeAbsAddrMap); *\/ */
        /* someAdded = haveMappedAbsAddrSetsChanged(uivMap, JITFALSE); */
    }
    return someAdded;
}


/**
 * Initialise the uiv map between caller and callee and also initialise
 * returned values too.  Returns JITTRUE if the initialisation added
 * anything to any set.
 */
static JITBOOLEAN
initialiseCalleeUivMaps(method_info_t *callerInfo, SmallHashTable *methodsInfo) {
    JITBOOLEAN someAdded = JITFALSE;
    SmallHashTableItem *callItem = smallHashTableFirst(callerInfo->calleeUivMaps);

    /* Iterate over all callees. */
    while (callItem) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callItem->key);
        if (calleeInfo) {
            SmallHashTable *uivMap = callItem->element;
            someAdded |= initialiseOneCalleeUivMap(callerInfo, calleeInfo, uivMap);
        }
        callItem = smallHashTableNext(callerInfo->calleeUivMaps, callItem);
    }

    /* Iterate over all call sites. */
    callItem = smallHashTableFirst(callerInfo->callMethodMap);
    while (callItem) {
        ir_instruction_t *callInst = callItem->key;
        if (callInst->type == IRCALL) {
            someAdded |= initialiseCalleeUivMapFromCall(callerInfo, callInst, callItem->element, methodsInfo);
        } else {
            XanList *callSiteList = callItem->element;
            XanListItem *currCallSite = xanList_first(callSiteList);
            while (currCallSite) {
                call_site_t *callSite = currCallSite->data;
                someAdded |= initialiseCalleeUivMapFromCall(callerInfo, callInst, callSite, methodsInfo);
                currCallSite = currCallSite->next;
            }
        }

        /* Next call instruction. */
        callItem = smallHashTableNext(callerInfo->callMethodMap, callItem);
    }

    /* Return whether anything was added. */
    return someAdded;
}


/**
 * Determine whether a uiv is from a non-variable or a parameter.
 */
static JITBOOLEAN
isUivNonVarOrParam(uiv_t *uiv, ir_method_t *method) {
    return !isBaseUivFromVar(uiv) || IRMETHOD_isTheVariableAMethodParameter(method, getVarFromBaseUiv(uiv));
}


/**
 * Transfer merges from one callee method to a caller.
 */
static void
transferAbsAddrMergesFromOneCallee(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *uivMap) {
    SmallHashTableItem *mergeSetItem = smallHashTableFirst(calleeInfo->mergeAbsAddrMap);
    while (mergeSetItem) {
        uiv_t *calleeUiv = mergeSetItem->key;
        if (isUivNonVarOrParam(calleeUiv, calleeInfo->ssaMethod)) {
            aa_mapped_set_t *callerFromMap = smallHashTableLookup(uivMap, calleeUiv);
            if (callerFromMap) {
                VSet *mergeSet = mergeSetItem->element;
                VSetItem *mergeItem = vSetFirst(mergeSet);
                while (mergeItem) {
                    aa_merge_t *merge = getSetItemData(mergeItem);
                    if (merge->type == AA_MERGE_UIVS) {
                        aa_mapped_set_t *callerToMap = smallHashTableLookup(uivMap, merge->uiv);
                        if (callerToMap) {
                            abstract_addr_t *aaFrom = absAddrSetFirst(callerFromMap->aaSet);
                            while (aaFrom) {
                                abstract_addr_t *aaTo = absAddrSetFirst(callerToMap->aaSet);
                                while (aaTo) {
                                    addGenericAbsAddrUivMerge(aaFrom->uiv, aaTo->uiv, callerInfo->mergeAbsAddrMap);
                                    aaTo = absAddrSetNext(callerToMap->aaSet, aaTo);
                                }
                                aaFrom = absAddrSetNext(callerFromMap->aaSet, aaFrom);
                            }
                        }
                    } else {
                        abstract_addr_t *aaFrom = absAddrSetFirst(callerFromMap->aaSet);
                        assert(merge->to->uiv == calleeUiv);
                        while (aaFrom) {
                            addGenericAbsAddrOffsetMerge(aaFrom, merge->stride, callerInfo->mergeAbsAddrMap);
                            aaFrom = absAddrSetNext(callerFromMap->aaSet, aaFrom);
                        }
                    }
                    mergeItem = vSetNext(mergeSet, mergeItem);
                }
            }
        }
        mergeSetItem = smallHashTableNext(calleeInfo->mergeAbsAddrMap, mergeSetItem);
    }
}


/**
 * Transfer uiv merge targets from one callee method to a caller.
 */
static void
transferUivMergeTargetsFromOneCallee(method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *uivMap) {
    SmallHashTableItem *uivItem = smallHashTableFirst(calleeInfo->uivMergeTargets);
    while (uivItem) {
        uiv_t *calleeUiv = uivItem->key;
        if (isUivNonVarOrParam(calleeUiv, calleeInfo->ssaMethod)) {
            aa_mapped_set_t *callerMap = smallHashTableLookup(uivMap, calleeUiv);
            if (callerMap) {
                aa_merge_type_t mergeType = fromPtr(JITNUINT, uivItem->element);
                abstract_addr_t *aaCaller = absAddrSetFirst(callerMap->aaSet);
                while (aaCaller) {
                    addUivMergeTarget(callerInfo->uivMergeTargets, aaCaller->uiv, mergeType);
                    aaCaller = absAddrSetNext(callerMap->aaSet, aaCaller);
                }
            }
        }
        uivItem = smallHashTableNext(calleeInfo->uivMergeTargets, uivItem);
    }
}


/**
 * Transfer merges from each callee to the given method.  It's important to
 * transfer these because the abstract addresses in the callees will have been
 * merged according to their local merges and we need to make sure all abstract
 * addresses in the caller are also merged in the same way to provide
 * consistency.  This avoids missing abstract addresses that should alias each
 * other.
 */
static void
transferAbsAddrMergesFromCallees(method_info_t *callerInfo, SmallHashTable *methodsInfo, JITUINT32 iteration) {
    SmallHashTableItem *callItem = smallHashTableFirst(callerInfo->calleeUivMaps);
    while (callItem) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callItem->key);
        if (calleeInfo && (iteration == 0 || calleeInfo->memAbsAddrsModified)) {
            SmallHashTable *uivMap = callItem->element;
            transferAbsAddrMergesFromOneCallee(callerInfo, calleeInfo, uivMap);
        }
        callItem = smallHashTableNext(callerInfo->calleeUivMaps, callItem);
    }
}


/**
 * Transfer uivs that are the targets of merges from each callee to the given
 * method.  It's important to do this tranfer because merges within abstract
 * address sets are performed locally.
 */
static void
transferUivMergeTargetsFromCallees(method_info_t *callerInfo, SmallHashTable *methodsInfo, JITUINT32 iteration) {
    SmallHashTableItem *callItem = smallHashTableFirst(callerInfo->calleeUivMaps);
    while (callItem) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callItem->key);
        if (calleeInfo && (iteration == 0 || calleeInfo->memAbsAddrsModified)) {
            SmallHashTable *uivMap = callItem->element;
            transferUivMergeTargetsFromOneCallee(callerInfo, calleeInfo, uivMap);
        }
        callItem = smallHashTableNext(callerInfo->calleeUivMaps, callItem);
    }
}


/**
 * Transfer mappings from each callee to the given method.  These are the
 * mappings from callee uivs to sets of caller abstract addresses, as well
 * as points-to arcs in each callee being reflected in the caller.
 */
static void
transferMappingsFromCallees(method_info_t *callerInfo, SmallHashTable *methodsInfo, XanList *scc) {
    if (smallHashTableSize(callerInfo->callMethodMap) > 0) {
        JITBOOLEAN additions = JITTRUE;
        JITUINT32 iteration = 0;
        FIXED_POINT_ITER_START;

        /* Continue iterating whilst there are additions. */
        while (additions) {
            FIXED_POINT_ITER_NEW(100 * smallHashTableSize(callerInfo->callMethodMap), "transferring points-tos");

            /* Reset termination condition. */
            additions = JITFALSE;

            /* Initialise the uiv maps between each call and its callees. */
            /* copyMappedAbsAddrSets(callerInfo->memAbsAddrMap); */
            if (initialiseCalleeUivMaps(callerInfo, methodsInfo)) {
                PDEBUG("  Changes initialising callee uiv maps\n");
                additions = JITTRUE;
            }

            /* Copy over merges. */
            transferAbsAddrMergesFromCallees(callerInfo, methodsInfo, iteration);
            transferUivMergeTargetsFromCallees(callerInfo, methodsInfo, iteration);

            /* Merge the uiv maps. */
            /* mergeMappedAbsAddrSets(callerInfo->memAbsAddrMap, callerInfo->uivMergeTargets); */
            /* if (haveMappedAbsAddrSetsChanged(callerInfo->memAbsAddrMap, JITTRUE)) { */
            /*     additions = JITTRUE; */
            /*     PDEBUG("  Memory abstract address sets have changed\n"); */
            /* } */

            /* Augment the callee uiv maps and caller points-to sets. */
            copyMappedAbsAddrSetsTable(callerInfo->memAbsAddrMap);
            if (augmentCallerMappingsFromCallees(callerInfo, methodsInfo, scc)) {
                PDEBUG("  Changes augmenting caller mappings\n");
                additions = JITTRUE;
            }

            /* Merge the memory maps. */
            PDEBUGNL("Merging and checking memory abstract address sets\n");
            mergeMappedAbsAddrSetsTable(callerInfo->memAbsAddrMap, callerInfo->uivMergeTargets);
            PDEBUGNL("Finished merging, now checking memory abstract address sets\n");
            if (haveMappedAbsAddrSetsChangedTable(callerInfo->memAbsAddrMap, JITTRUE)) {
                callerInfo->memAbsAddrsModified = JITTRUE;
                additions = JITTRUE;
                PDEBUG("  Memory abstract address sets have changed\n");
            } else {
                callerInfo->memAbsAddrsModified = JITFALSE;
            }

            /* For now, let's see what happens if we just do this once. */
            /* additions = JITFALSE; */

            /* Debugging. */
            printMethodMemAbstractAddresses(callerInfo);
            iteration += 1;
        }
        FIXED_POINT_ITER_END;
        PDEBUGLITE("  Taken %u iterations\n", fixedPointIterNum);

        /* Clear the cache of mappings from callees to caller. */
        clearCalleeToCallerAbsAddrMap(callerInfo->calleeToCallerAbsAddrMap);

        /* Mark call instructions as definers of certain uivs. */
        markUivDefinesAcrossCalls(callerInfo, methodsInfo);
    }
}


/**
 * Update memory abstract address sets that access a whole array, by copying
 * the abstract addresses in elements of the array into the whole array set.
 */
static void
updateWholeArrayMemAbsAddrSets(method_info_t *info) {
    SmallHashTableItem *mapItem = smallHashTableFirst(info->memAbsAddrMap);
    while (mapItem) {
        abstract_addr_t *wholeArrayKey = mapItem->key;
        if (wholeArrayKey->offset == WHOLE_ARRAY_OFFSET) {
            aa_mapped_set_t *wholeArrayMapping = mapItem->element;
            abs_addr_set_t *elementKeys = getKeyAbsAddrSetFromUivKey(wholeArrayKey->uiv, info);
            abstract_addr_t *aaKey = absAddrSetFirst(elementKeys);
            while (aaKey) {
                if (aaKey->offset != WHOLE_ARRAY_OFFSET) {
                    aa_mapped_set_t *elementMapping = smallHashTableLookup(info->memAbsAddrMap, aaKey);
                    absAddrSetAppendSet(wholeArrayMapping->aaSet, elementMapping->aaSet);
                }
                aaKey = absAddrSetNext(elementKeys, aaKey);
            }
            freeAbstractAddressSet(elementKeys);
            absAddrSetMerge(wholeArrayMapping->aaSet, info->uivMergeTargets);
        }
        mapItem = smallHashTableNext(info->memAbsAddrMap, mapItem);
    }
}


/**
 * Perform intraprocedural and interprocedural analysis within all methods from
 * a single SCC.  The initial uivs and abstract address sets for each variable
 * have already been calculated.
 *
 * We perform the intraprocedural analysis until a fixed point is reached for
 * the whole SCC.  This allows us to create the points-to relations within each
 * method, but also transfer abstract address sets between methods via call &
 * return instructions.  It is important that these sets are available for use
 * when creating the points-to relations.
 *
 * Interprocedural analysis is also performed to a fixed point, but for each
 * method alone, rather than the whole SCC.
 */
static void
performAnalysisInSCC(XanList *scc, SmallHashTable *methodsInfo) {
    XanListItem *currMethod;
    JITBOOLEAN additions = JITTRUE;
    FIXED_POINT_ITER_START;

    /* Intraprocedural analysis. */
    PDEBUGLITE("Intraprocedural Analysis\n");
    while (additions) {
        FIXED_POINT_ITER_NEW(350 * xanList_length(scc), "scc intraprocedural analysis");

        /* Reset termination condition. */
        additions = JITFALSE;

        /* Analyse each method in the scc. */
        currMethod = xanList_first(scc);
        while (currMethod) {
            ir_method_t *method = currMethod->data;
            method_info_t *info = smallHashTableLookup(methodsInfo, method);
            GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug);
            if (enableExtendedDebugPrinting(method)) {
                SET_DEBUG_ENABLE(JITTRUE);
            }
            PDEBUGLITE("Analysis for %s\n", IRMETHOD_getMethodName(info->ssaMethod));

            /* Create points-to relations. */
            if (createPointsTosInMethod(info, methodsInfo)) {
                PDEBUG("  Changes creating points-tos\n");
                additions = JITTRUE;
            }

            /* Initialise the uiv maps between each call and its callees.  This is
               done here to allow the transfer of abstract address sets from call
               & return instructions.  It is also called each iteration within
               transferMappingsFromCallees(), although there may be a way to remove
               it from there, or at least only do part of it. */
            if (initialiseCalleeUivMaps(info, methodsInfo)) {
                PDEBUG("  Changes initialising uiv maps\n");
                additions = JITTRUE;
            }

            /* Update the set of abstract addresses to transfer from this method. */
            if (updateAbsAddrSetToTransfer(info)) {
                PDEBUG("  Changes updating transferred abstract addresses\n");
                additions = JITTRUE;
            }

            /* Clear the cache of mappings from callee to caller. */
            clearCalleeToCallerAbsAddrMap(info->calleeToCallerAbsAddrMap);

            /* Next method. */
            SET_DEBUG_ENABLE(prevEnablePrintDebug);
            currMethod = xanList_next(currMethod);
        }
    }
    FIXED_POINT_ITER_END;
    /* smallHashTablePrintMemUse(); */
    /* vllpaTypesPrintMemUse(); */
    /* vllpaPrintMemUse(); */

    /* Interprocedural analysis. */
    PDEBUGLITE("Interprocedural Analysis\n");
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug);
        if (enableExtendedDebugPrinting(method)) {
            SET_DEBUG_ENABLE(JITTRUE);
        }
        PDEBUGLITE("Analysis for %s\n", IRMETHOD_getMethodName(info->ssaMethod));

        /* Debugging. */
        printMethodVarAbstractAddresses(info);
        printMethodMemAbstractAddresses(info);

        /* Perform the analysis. */
        transferMappingsFromCallees(info, methodsInfo, scc);

        /* Update any memory abstract address sets accessing a whole array. */
        updateWholeArrayMemAbsAddrSets(info);

        /* There may well be more globals to transfer now. */
        updateAbsAddrSetToTransfer(info);
        /* smallHashTablePrintMemUse(); */
        /* vllpaTypesPrintMemUse(); */
        /* vllpaPrintMemUse(); */

        /* Next method. */
        SET_DEBUG_ENABLE(prevEnablePrintDebug);
        currMethod = xanList_next(currMethod);
    }

    /* Debugging. */
#ifdef PRINTDEBUG
    PDEBUG("Post-analysis abstract addresses\n");
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug);
        if (enableExtendedDebugPrinting(method)) {
            SET_DEBUG_ENABLE(JITTRUE);
        }
        printMethodUivs(info);
        printMethodVarAbstractAddresses(info);
        printMethodMemAbstractAddresses(info);
        printMethodGenericMergeMap(info);
        PDEBUGNL("Transfer set: ");
        printAbstractAddressSet(info->initialAbsAddrTransferSet);
        SET_DEBUG_ENABLE(prevEnablePrintDebug);
        currMethod = xanList_next(currMethod);
    }
#endif
}


/**
 * Clear the methods information structure across one call site if the
 * callee has been analysed by all callers.
 */
static void
clearMethodsInfoAcrossCallSite(call_site_t *callSite, SmallHashTable *methodsInfo) {
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);

        /* One more caller has processed this method. */
        calleeInfo->callersProcessed += 1;

        /* Check whether this method is no longer needed. */
        if (calleeInfo->callersProcessed == calleeInfo->numCallers) {
            /* clearEphemeralInfoMappings(calleeInfo); */
        }
    }
}


/**
 * Within an SCC, check each method and the methods that it calls, marking
 * the callees as having been analysed by one more caller.  Once all
 * callers have analysed a method, clear out most of its methods
 * information structure.  This helps to keep in memory only the data that
 * is actually needed, saving space, especially since it won't be needed
 * again but can be regenerated.
 */
static void
clearCalleeMethodsInfosInSCC(XanList *scc, SmallHashTable *methodsInfo) {
    XanListItem *currMethod;

    /* Check each method in the SCC. */
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        SmallHashTableItem *currCall;

        /* Check each method called by this one. */
        currCall = smallHashTableFirst(info->callMethodMap);
        while (currCall) {
            if (((ir_instruction_t *)currCall->key)->type == IRCALL) {
                clearMethodsInfoAcrossCallSite(currCall->element, methodsInfo);
            } else {
                XanList *callSiteList = currCall->element;
                XanListItem *currCallSite = xanList_first(callSiteList);
                while (currCallSite) {
                    call_site_t *callSite = currCallSite->data;
                    clearMethodsInfoAcrossCallSite(callSite, methodsInfo);
                    currCallSite = currCallSite->next;
                }
            }
            currCall = smallHashTableNext(info->callMethodMap, currCall);
        }
        currMethod = xanList_next(currMethod);
    }
}


/**
 * Print out the abstract addresses that icall instructions use to get
 * their function pointers from.
 */
#ifdef PRINTDEBUG
static void
printICallAddresses(XanList *scc, SmallHashTable *methodsInfo) {
    XanListItem *currMethod;

    /* Check each method in the SCC. */
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        JITUINT32 i;
        for (i = 0; i< IRMETHOD_getInstructionsNumber(info->ssaMethod); ++i) {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
            ir_item_t *param = IRMETHOD_getInstructionParameter2(inst);
            if (inst->type == IRICALL && param->type == IROFFSET) {
                PDEBUG("ICall at inst %u from var %lld: ", i, IRMETHOD_getInstructionParameter2Value(inst));
                abs_addr_set_t *srcSet = getVarAbstractAddressSet(param, info, JITFALSE);
                printAbstractAddressSet(srcSet);
            }
        }
        currMethod = xanList_next(currMethod);
    }
}
#else
#define printICallAddresses(scc, methodsInfo)
#endif  /* ifdef PRINTDEBUG */


/**
 * Create a set for holding function pointers within a map with the given
 * abstract address as the key.
 */
static func_ptr_resolve_set_t *
createMappingFromAbsAddrToFuncPtrSet(abstract_addr_t *aaPtr, SmallHashTable *absAddrMap, method_info_t *info, JITBOOLEAN pointer) {
    /* PDEBUG("Func ptr abstract address holder: "); */
    /* printAbstractAddress(aaPtr, JITTRUE); */

    /* Create a new set for this abstract address if there isn't one already. */
    if (!smallHashTableLookup(absAddrMap, aaPtr)) {
        func_ptr_resolve_set_t *resolveSet = vllpaAllocFunction(sizeof(func_ptr_resolve_set_t), 5);
        resolveSet->pointer = pointer;
        resolveSet->info = info;
        resolveSet->funcPtrSet = newAbstractAddressSetEmpty();
        smallHashTableInsert(absAddrMap, aaPtr, resolveSet);
        PDEBUG("Func ptr abstract address holder: ");
        printAbstractAddress(aaPtr, JITTRUE);
        return resolveSet;
    }
    return NULL;
}


/**
 * Identify abstract addresses that could be function pointers and set up
 * mappings within each method's information structure to sets of sets
 * that will be filled with function pointers as they are discovered.
 */
static void
identifyFuncPtrAbsAddrsInMethod(method_info_t *info) {
    JITUINT32 i;
    PDEBUG("Identifying function pointers in %s\n", IRMETHOD_getMethodName(info->ssaMethod));
    for (i = 0; i< IRMETHOD_getInstructionsNumber(info->ssaMethod); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        ir_item_t *param = IRMETHOD_getInstructionParameter2(inst);

        /* Find indirect calls. */
        if (inst->type == IRICALL && param->type == IROFFSET) {
            abs_addr_set_t *srcSet = getVarAbstractAddressSet(param, info, JITFALSE);
            abstract_addr_t *aaNonFuncPtr = absAddrSetFirst(srcSet);
            SmallHashTable *absAddrMap = smallHashTableLookup(info->funcPtrAbsAddrMap, inst);

            /* Create new abstract address map if there isn't one already. */
            if (!absAddrMap) {
                absAddrMap = allocateNewHashTable();
                smallHashTableInsert(info->funcPtrAbsAddrMap, inst, absAddrMap);
                PDEBUG("Created new abstract address map\n");
            }

            /* Create mappings for each abstract address. */
            while (aaNonFuncPtr) {
                if (!isAbstractAddressAFunctionPointer(aaNonFuncPtr)) {
                    abs_addr_set_t *ptrsToFuncPtrSets = getKeyAbsAddrSetFromAbsAddr(aaNonFuncPtr, info);
                    PDEBUG("  Not a function pointer ");
                    printAbstractAddress(aaNonFuncPtr, JITTRUE);

                    /* Check that some abstract address sets are found. */
                    if (!absAddrSetIsEmpty(ptrsToFuncPtrSets)) {
                        abstract_addr_t *aaPtr = absAddrSetFirst(ptrsToFuncPtrSets);
                        PDEBUG("      Is in memory\n");
                        while (aaPtr) {
                            createMappingFromAbsAddrToFuncPtrSet(aaPtr, absAddrMap, info, JITTRUE);
                            aaPtr = absAddrSetNext(ptrsToFuncPtrSets, aaPtr);
                        }
                    }

                    /* This abstract address isn't in memory. */
                    else {
                        PDEBUG("    Not in memory\n");
                        assert(IRMETHOD_isTheVariableAMethodParameter(info->ssaMethod, getVarFromBaseUiv(aaNonFuncPtr->uiv)));
                        createMappingFromAbsAddrToFuncPtrSet(aaNonFuncPtr, absAddrMap, info, JITFALSE);
                    }
                    freeAbstractAddressSet(ptrsToFuncPtrSets);
                }

                /* Next abstract address. */
                aaNonFuncPtr = absAddrSetNext(srcSet, aaNonFuncPtr);
            }
        }
    }
}


/**
 * Get a set of function pointer abstract addresses that are pointed to by
 * the given abstract address in memory.  Returns NULL if no function
 * pointers are found.
 */
static abs_addr_set_t *
getFuncPtrSetFromMemAbsAddr(abstract_addr_t *aa, method_info_t *info) {
    abs_addr_set_t *funcPtrSet = NULL;
    abs_addr_set_t *memSet = getMemAbstractAddressSet(aa, info, JITTRUE);
    abstract_addr_t *funcPtr = absAddrSetFirst(memSet);
    PDEBUG("Getting function pointers from memory set ");
    printAbstractAddress(aa, JITFALSE);
    PDEBUGB(": ");
    printAbstractAddressSet(memSet);
    while (funcPtr) {
        if (isBaseUivFromFunc(funcPtr->uiv)) {
            PDEBUG("Found func ptr: ");
            printAbstractAddress(funcPtr, JITTRUE);
            if (!funcPtrSet) {
                funcPtrSet = newAbstractAddressSet(funcPtr);
            } else {
                absAddrSetAppendAbsAddr(funcPtrSet, funcPtr);
            }
        }
        funcPtr = absAddrSetNext(memSet, funcPtr);
    }
    return funcPtrSet;
}


/**
 * Get a set of function pointer abstract addresses that are assigned to
 * the given variable abstract address.  Returns NULL if no function
 * pointers are found.
 */
static abs_addr_set_t *
getFuncPtrSetFromVarAbsAddr(abstract_addr_t *aa, method_info_t *info) {
    abs_addr_set_t *funcPtrSet = NULL;
    abs_addr_set_t *varSet = getVarAbstractAddressSetVarKey(getVarFromBaseUiv(aa->uiv), info, JITTRUE);
    abstract_addr_t *funcPtr = absAddrSetFirst(varSet);
    PDEBUG("Getting function pointers from variable set ");
    printAbstractAddress(aa, JITFALSE);
    PDEBUGB(": ");
    printAbstractAddressSet(varSet);
    while (funcPtr) {
        if (isBaseUivFromFunc(funcPtr->uiv)) {
            PDEBUG("Found func ptr: ");
            printAbstractAddress(funcPtr, JITTRUE);
            if (!funcPtrSet) {
                funcPtrSet = newAbstractAddressSet(funcPtr);
            } else {
                absAddrSetAppendAbsAddr(funcPtrSet, funcPtr);
            }
        }
        funcPtr = absAddrSetNext(varSet, funcPtr);
    }
    return funcPtrSet;
}


/**
 * Propagate abstract addresses that could be function pointers across a call
 * site.
 */
static JITBOOLEAN
identifyFuncPtrAbsAddrsAcrossCallSite(call_site_t *callSite, method_info_t *callerInfo, SmallHashTable *methodsInfo) {
    JITBOOLEAN additions = JITFALSE;
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        if (calleeInfo != callerInfo) {
            SmallHashTable *calleeUivMap = smallHashTableLookup(callerInfo->calleeUivMaps, callSite->method);
            SmallHashTableItem *currAbsAddrMap = smallHashTableFirst(calleeInfo->funcPtrAbsAddrMap);
            PDEBUG("Callee %s\n", IRMETHOD_getMethodName(calleeInfo->ssaMethod));

            /* Check all mappings in the callee. */
            while (currAbsAddrMap) {
                ir_instruction_t *callInst = currAbsAddrMap->key;

                /* Don't add mappings that come back from yourself. */
                if (!IRMETHOD_doesInstructionBelongToMethod(callerInfo->ssaMethod, callInst)) {
                    SmallHashTable *calleeAbsAddrMap = currAbsAddrMap->element;
                    SmallHashTableItem *currResolveSet = smallHashTableFirst(calleeAbsAddrMap);
                    SmallHashTable *callerAbsAddrMap = smallHashTableLookup(callerInfo->funcPtrAbsAddrMap, callInst);

                    /* Create a new abstract address map if this doesn't already exist. */
                    if (!callerAbsAddrMap) {
                        callerAbsAddrMap = allocateNewHashTable();
                        smallHashTableInsert(callerInfo->funcPtrAbsAddrMap, callInst, callerAbsAddrMap);
                    }

                    /* Check all resolve sets in the callee. */
                    while (currResolveSet) {
                        abstract_addr_t *aaCallee = currResolveSet->key;
                        func_ptr_resolve_set_t *calleeResolveSet = currResolveSet->element;
                        abs_addr_set_t *callerSet = mapCalleeAbsAddrToCallerAbsAddrSet(aaCallee, callerInfo, calleeInfo, calleeUivMap, JITTRUE);

                        /* This may map to nothing. */
                        if (callerSet) {
                            PDEBUG("Func ptr abstract address holder from %s, mapped ", IRMETHOD_getMethodName(calleeInfo->ssaMethod));
                            printAbstractAddress(aaCallee, JITFALSE);
                            PDEBUGB(" to ");
                            printAbstractAddressSet(callerSet);
                            abstract_addr_t *aaCaller = absAddrSetFirst(callerSet);
                            while (aaCaller) {
                                PDEBUG("Func ptr abstract address holder: ");
                                printAbstractAddress(aaCaller, JITTRUE);

                                /* It may map to a function pointer. */
                                if (isAbstractAddressAFunctionPointer(aaCaller)) {
                                    if (!absAddrSetContains(calleeResolveSet->funcPtrSet, aaCaller)) {
                                        absAddrSetAppendAbsAddr(calleeResolveSet->funcPtrSet, aaCaller);
                                        PDEBUG("Resolved func ptr: ");
                                        printAbstractAddress(aaCallee, JITFALSE);
                                        PDEBUGB(" -> ");
                                        printAbstractAddressSet(calleeResolveSet->funcPtrSet);
                                        additions = JITTRUE;
                                    }
                                }

                                /* Add the callee's resolve set to this map, if it doesn't already exist. */
                                else if (!smallHashTableLookup(callerAbsAddrMap, aaCaller)) {
                                    smallHashTableInsert(callerAbsAddrMap, aaCaller, calleeResolveSet);
                                    /* PDEBUG("Inserted mapping between "); */
                                    /* printAbstractAddress(aaCaller, JITFALSE); */
                                    /* PDEBUGB(" and "); */
                                    /* printAbstractAddressSet(calleeResolveSet->funcPtrSet); */
                                    additions = JITTRUE;
                                }
                                aaCaller = absAddrSetNext(callerSet, aaCaller);
                            }
                            freeAbstractAddressSet(callerSet);
                        }
                        currResolveSet = smallHashTableNext(calleeAbsAddrMap, currResolveSet);
                    }
                }
                currAbsAddrMap = smallHashTableNext(calleeInfo->funcPtrAbsAddrMap, currAbsAddrMap);
            }
        }
    }
    return additions;
}


/**
 * Propagate abstract addresses that could be function pointers from callee
 * methods.  Create mappings for them in this method's information
 * structure.  Returns JITTRUE if there are any additions to any of the
 * sets.
 */
static JITBOOLEAN
identifyFuncPtrAbsAddrsInCallees(method_info_t *info, SmallHashTable *methodsInfo) {
    SmallHashTableItem *currCall;
    JITBOOLEAN additions = JITFALSE;
    PDEBUG("Identifying function pointers from callees to %s\n", IRMETHOD_getMethodName(info->ssaMethod));

    /* Check each method called by this one for new mappings. */
    currCall = smallHashTableFirst(info->callMethodMap);
    while (currCall) {
        if (((ir_instruction_t *)currCall->key)->type == IRCALL) {
            additions |= identifyFuncPtrAbsAddrsAcrossCallSite(currCall->element, info, methodsInfo);
        } else {
            XanList *callSiteList = currCall->element;
            XanListItem *currCallSite = xanList_first(callSiteList);
            while (currCallSite) {
                call_site_t *callSite = currCallSite->data;
                additions |= identifyFuncPtrAbsAddrsAcrossCallSite(callSite, info, methodsInfo);
                currCallSite = currCallSite->next;
            }
        }
        currCall = smallHashTableNext(info->callMethodMap, currCall);
    }

    /* Return whether there were additions. */
    return additions;
}



/**
 * Resolve function pointers, if possible, from this method or callees.
 * Returns JITTRUE if any resolutions occurred.
 */
static JITBOOLEAN
resolveFuncPtrAbsAddrs(method_info_t *info, SmallHashTable *methodsInfo) {
    SmallHashTableItem *currAbsAddrMap;
    JITBOOLEAN resolved = JITFALSE;

    /* Try to resolve function pointers. */
    currAbsAddrMap = smallHashTableFirst(info->funcPtrAbsAddrMap);
    while (currAbsAddrMap) {
        ir_instruction_t *icall = currAbsAddrMap->key;
        SmallHashTable *absAddrMap = currAbsAddrMap->element;
        XanList *resolveKeyList = smallHashTableToKeyList(absAddrMap);
        XanListItem *aaItem = xanList_first(resolveKeyList);
        while (aaItem) {
            abstract_addr_t *aaPtr = aaItem->data;
            func_ptr_resolve_set_t *resolveSet = smallHashTableLookup(absAddrMap, aaPtr);
            abs_addr_set_t *funcPtrSet;

            /* Get the correct type of function pointer set. */
            if (resolveSet->pointer) {
                funcPtrSet = getFuncPtrSetFromMemAbsAddr(aaPtr, info);
            } else {
                funcPtrSet = getFuncPtrSetFromVarAbsAddr(aaPtr, info);
            }

            /* No set returned if there are no mappings. */
            if (funcPtrSet) {
                if (absAddrSetTrackAppendSet(resolveSet->funcPtrSet, funcPtrSet)) {
                    PDEBUG("Resolved func ptr: ");
                    printAbstractAddress(aaPtr, JITFALSE);
                    PDEBUGB(" -> ");
                    printAbstractAddressSet(resolveSet->funcPtrSet);
                    resolved = JITTRUE;
                }
                freeAbstractAddressSet(funcPtrSet);
            }

            /* If this was a merge target, consider merged-away mappings. */
            if (resolveSet->pointer) {
                abs_addr_set_t *memKeys = getMemKeyAbsAddrSetFromMergedUivKey(aaPtr->uiv, info);
                abstract_addr_t *aaKey = absAddrSetFirst(memKeys);
                while (aaKey) {
                    func_ptr_resolve_set_t *keyResolveSet = createMappingFromAbsAddrToFuncPtrSet(aaKey, absAddrMap, resolveSet->info, JITTRUE);
                    if (keyResolveSet) {
                        xanList_append(resolveKeyList, aaKey);
                        if (resolveSet->info != info) {
                            assert(IRMETHOD_doesInstructionBelongToMethod(resolveSet->info->ssaMethod, icall));
                            SmallHashTable *icallMethodAbsAddrMap = smallHashTableLookup(resolveSet->info->funcPtrAbsAddrMap, icall);
                            if (!smallHashTableLookup(icallMethodAbsAddrMap, aaKey)) {
                                smallHashTableInsert(icallMethodAbsAddrMap, aaKey, keyResolveSet);
                            }
                        }
                    }
                    aaKey = absAddrSetNext(memKeys, aaKey);
                }
                freeAbstractAddressSet(memKeys);
            }
            aaItem = aaItem->next;
        }
        freeListUnallocated(resolveKeyList);
        currAbsAddrMap = smallHashTableNext(info->funcPtrAbsAddrMap, currAbsAddrMap);
    }

    /* Return whether there were resolutions. */
    return resolved;
}


/**
 * Identify abstract addresses that could be function pointers and set up
 * mappings within each method's information structure to sets of sets
 * that will be filled with function pointers as they are discovered.  Then
 * consider each callee method and propagate down function pointers that
 * the callee and its callees need to be resolved.  Do this for each method
 * within a single SCC.
 */
static void
identifyFuncPtrsInSCC(XanList *scc, SmallHashTable *methodsInfo) {
    XanListItem *currMethod;
    JITBOOLEAN changes;
    FIXED_POINT_ITER_START;
    /*   GET_DEBUG_ENABLE(JITBOOLEAN oldEnablePDebug); */
    /*   SET_DEBUG_ENABLE(JITTRUE); */

    /* Identify function pointer abstract addresses within each method. */
    PDEBUG("Identifying Function Pointers\n");
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug);
        if (enableExtendedDebugPrinting(method)) {
            SET_DEBUG_ENABLE(JITTRUE);
        }
        PDEBUGLITE("Identifying in %s\n", IRMETHOD_getMethodName(info->ssaMethod));
        identifyFuncPtrAbsAddrsInMethod(info);
        SET_DEBUG_ENABLE(prevEnablePrintDebug);
        currMethod = xanList_next(currMethod);
    }

    /* Transfer abstract addresses needing resolution from callees. */
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug);
        if (enableExtendedDebugPrinting(method)) {
            SET_DEBUG_ENABLE(JITTRUE);
        }
        PDEBUG("Resolving in %s\n", IRMETHOD_getMethodName(info->ssaMethod));
        changes = JITTRUE;
        while (changes) {
            FIXED_POINT_ITER_NEW(4 * xanList_length(scc), "scc function pointer identification");
            changes = JITFALSE;
            if (identifyFuncPtrAbsAddrsInCallees(info, methodsInfo)) {
                PDEBUG("  Changes identifying in callees\n");
                changes = JITTRUE;
            }
            if (resolveFuncPtrAbsAddrs(info, methodsInfo)) {
                PDEBUG("  Changes resolving function pointer abstract addresses\n");
                changes = JITTRUE;
            }
        }
        FIXED_POINT_ITER_END;
        SET_DEBUG_ENABLE(prevEnablePrintDebug);
        currMethod = xanList_next(currMethod);
    }
    /*   currMethod = vSetFirst(scc); */
    /*   while (currMethod) { */
    /*     ir_method_t *method = getSetItemData(currMethod); */
    /*     method_info_t *info = smallHashTableLookup(methodsInfo, method); */
    /*     SmallHashTableItem *currAbsAddrMap = smallHashTableFirst(info->funcPtrAbsAddrMap); */
    /*     PDEBUGNL("Func Ptr Mappings for %s\n", IRMETHOD_getMethodName(info->ssaMethod)); */
    /*     while (currAbsAddrMap) { */
    /*       ir_instruction_t *inst = currAbsAddrMap->key; */
    /*       SmallHashTable *absAddrMap = currAbsAddrMap->element; */
    /*       SmallHashTableItem *currResolveSet = smallHashTableFirst(absAddrMap); */
    /*       PDEBUG("Inst %u %sfrom this method\n", inst->ID, IRMETHOD_doesInstructionBelongToMethod(info->ssaMethod, inst) ? "" : "not "); */
    /*       while (currResolveSet) { */
    /*         abstract_addr_t *aaPtr = currResolveSet->key; */
    /*         func_ptr_resolve_set_t *resolveSet = currResolveSet->element; */
    /*         PDEBUG("  "); */
    /*         printAbstractAddress(aaPtr, JITFALSE); */
    /*         PDEBUGB(" to %p (%p)\n", resolveSet, resolveSet->funcPtrSet); */
    /*         currResolveSet = smallHashTableNext(absAddrMap, currResolveSet); */
    /*       } */
    /*       currAbsAddrMap = smallHashTableNext(info->funcPtrAbsAddrMap, currAbsAddrMap); */
    /*     } */
    /*     currMethod = vSetNext(scc, currMethod); */
    /*   } */
    /*   SET_DEBUG_ENABLE(oldEnablePDebug); */
}


/**
 * Perform phase 1 of the algorithm which is intraprocedural and then
 * interprocedural analysis.  After processing each SCC, callees of each
 * method are checked to see if they have transferred their memory abstract
 * addresses to all callers.  If so, their methods information structure
 * can be mostly cleared.
 */
static void
performAnalysis(XanList *allSccs, SmallHashTable *methodsInfo) {
    XanListItem *currScc;

    /* Debugging output. */
    PDEBUGNL("************************************************************\n");
    PDEBUGLITE("Performing Phase 1: Analysis\n");
    /* fprintf(stderr, "Performing Phase 1: Analysis\n"); */

    /* Iterate over all sccs and then all methods within each one. */
    currScc = xanList_first(allSccs);
    while (currScc) {
        XanList *scc = currScc->data;
        PDEBUGNL("\n");
        PDEBUG("New SCC\n");
        initialiseSCCUivsAndAbstractAddresses(scc, methodsInfo);
        performAnalysisInSCC(scc, methodsInfo);
        printICallAddresses(scc, methodsInfo);
        identifyFuncPtrsInSCC(scc, methodsInfo);
        /* smallHashTablePrintMemUse(); */
        /* vllpaTypesPrintMemUse(); */
        /* vllpaPrintMemUse(); */
        clearCalleeMethodsInfosInSCC(scc, methodsInfo);
        currScc = currScc->next;
        PDEBUG("Finished SCC analysis\n");
    }
}


/**
 * Link function pointers from the given set to the given indirect call
 * instruction.  Returns JITTRUE if any new links were made.
 */
static JITBOOLEAN
linkICallsToTargetsFromSet(method_info_t *callerInfo, abs_addr_set_t *aaSet, VSet *possibleCallees, XanList *callSiteList, SmallHashTable *methodsInfo) {
    JITBOOLEAN linked = JITFALSE;
    abstract_addr_t *funcPtr = absAddrSetFirst(aaSet);
    PDEBUG("  Linking icalls to targets in %s from set: ", IRMETHOD_getMethodName(callerInfo->ssaMethod));
    printAbstractAddressSet(aaSet);

    /* Convert the function pointers to methods. */
    while (funcPtr) {
        PDEBUG("    Considering ");
        printAbstractAddress(funcPtr, JITTRUE);
        if (isAbstractAddressAFunctionPointer(funcPtr)) {
            ir_method_t *callee = funcPtr->uiv->value.method;
            IRMETHOD_translateMethodToIR(callee);
            if (!IRPROGRAM_doesMethodBelongToALibrary(callee) && vSetContains(possibleCallees, callee)) {
                if (IRMETHOD_hasMethodInstructions(callee)) {
                    XanListItem *callSiteItem;
                    JITBOOLEAN found = JITFALSE;

                    /* Check if this method is already in the list of calls. */
                    callSiteItem = xanList_first(callSiteList);
                    while (callSiteItem && !found) {
                        call_site_t *callSite = callSiteItem->data;
                        found = callSite->method == callee;
                        callSiteItem = callSiteItem->next;
                    }

                    /* If not found, add a new call site. */
                    if (!found) {
                        call_site_t *callSite = vllpaAllocFunction(sizeof(call_site_t), 6);
                        callSite->method = callee;
                        if (!smallHashTableContains(callerInfo->calleeUivMaps, callee)) {
                            smallHashTableInsert(callerInfo->calleeUivMaps, callee, allocateNewHashTable());
                        }
                        xanList_append(callSiteList, callSite);

                        /* Create information structures too. */
                        createMethodInfo(callee, methodsInfo);
                        PDEBUG("    Added to list of calls\n");
                        PDEBUG("Resolved function pointer to %s\n", IRMETHOD_getCompleteMethodName(callee));
                        linked = JITTRUE;
                    }
                } else if (!vSetContains(callerInfo->emptyPossibleCallees, callee)) {
                    fprintf(stderr, "Warning: Possible callee %s has no method instructions\n", IRMETHOD_getCompleteMethodName(callee));
                    vSetInsert(callerInfo->emptyPossibleCallees, callee);
                }
            }
        }
        funcPtr = absAddrSetNext(aaSet, funcPtr);
    }

    /* Return whether new links were made. */
    return linked;
}


/**
 * Perform the final stage in function pointer resolution by actually
 * connecting the indirect calls with the methods that they target in
 * a specific method.  Returns JITTRUE if any new links were made.
 */
static JITBOOLEAN
linkICallsToTargetsInMethod(method_info_t *info, SmallHashTable *methodsInfo) {
    JITUINT32 i;
    JITBOOLEAN linked = JITFALSE;

    /* Find indirect calls. */
    PDEBUG("Finding indirect calls in %s\n", IRMETHOD_getMethodName(info->ssaMethod));
    for (i = 0; i< IRMETHOD_getInstructionsNumber(info->ssaMethod); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        ir_item_t *param = IRMETHOD_getInstructionParameter2(inst);
        if (inst->type == IRICALL && param->type == IROFFSET) {
            SmallHashTable *absAddrMap = smallHashTableLookup(info->funcPtrAbsAddrMap, inst);
            SmallHashTableItem *currResolveSet = smallHashTableFirst(absAddrMap);
            abs_addr_set_t *srcSet = getVarAbstractAddressSet(param, info, JITFALSE);
            XanList *callSiteList = smallHashTableLookup(info->callMethodMap, inst);
            XanList *callableMethods = IRMETHOD_getCallableMethods(inst);
            XanListItem *callItem = xanList_first(callableMethods);
            VSet *possibleCallees = allocateNewSet(12);
            PDEBUG("  Indirect call instruction %u through variable %lld\n", inst->ID, param->value.v);

            /* Set up the list of possible callees. */
            while (callItem) {
                IR_ITEM_VALUE calleeMethodId = (IR_ITEM_VALUE)(JITNUINT)callItem->data;
                ir_method_t *callee = IRMETHOD_getIRMethodFromMethodID(info->ssaMethod, calleeMethodId);
                vSetInsert(possibleCallees, callee);
                callItem = callItem->next;
            }

            /* Find the function pointers they could use from variables. */
            /* I think srcSet is always empty. */
            linked |= linkICallsToTargetsFromSet(info, srcSet, possibleCallees, callSiteList, methodsInfo);

            /* Find the function pointers they could use from propagation. */
            while (currResolveSet) {
                func_ptr_resolve_set_t *resolveSet = currResolveSet->element;
                linked |= linkICallsToTargetsFromSet(info, resolveSet->funcPtrSet, possibleCallees, callSiteList, methodsInfo);
                currResolveSet = smallHashTableNext(absAddrMap, currResolveSet);
            }

            /* Clean up. */
            freeSet(possibleCallees);
            freeListUnallocated(callableMethods);
        }
    }

    /* Return whether links were made. */
    return linked;
}


/**
 * Perform the final stage in function pointer resolution by actually
 * connecting the indirect calls with the methods that they target.
 * Returns JITTRUE if any new links were made.
 */
static JITBOOLEAN
linkICallsToTargets(XanList *allSccs, SmallHashTable *methodsInfo) {
    XanListItem *currScc;
    JITBOOLEAN linked = JITFALSE;
    /* GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug); */
    /* SET_DEBUG_ENABLE(JITTRUE); */

    /* Debugging output. */
    PDEBUGNL("************************************************************\n");
    PDEBUGLITE("Performing Phase 2: Resolving Function Pointers\n");
    /* fprintf(stderr, "Performing Phase 2: Resolving Function Pointers\n"); */

    /* Iterate over all sccs and then all methods within each one. */
    currScc = xanList_first(allSccs);
    while (currScc) {
        XanList *scc = currScc->data;
        XanListItem *currMethod = xanList_first(scc);
        while (currMethod) {
            ir_method_t *method = currMethod->data;
            method_info_t *info = smallHashTableLookup(methodsInfo, method);
            linked |= linkICallsToTargetsInMethod(info, methodsInfo);
            currMethod = xanList_next(currMethod);
        }
        currScc = xanList_next(currScc);
    }

    /* Return whether any links were made. */
    /* SET_DEBUG_ENABLE(prevEnablePrintDebug); */
    return linked;
}



/**
 * For each function pointer that has been propagated down to the main
 * method, check that there is an info object for it in the methods
 * information table.  This method is called once all function pointers
 * have been resolved and provides a sanity check that each function
 * pointer that is taken is actually used somewhere in the application.
 */
#if 0
static void
checkFunctionPointersResolved(ir_method_t *method, SmallHashTable *methodsInfo) {
    method_info_t *info = smallHashTableLookup(methodsInfo, method);
    XanList *uivList = smallHashTableLookup(info->uivs, toPtr(UIV_FUNC));
    XanListItem *uivItem = uivList->first(uivList);

    /* Check all function pointer uivs. */
    while (uivItem) {
        uiv_t *uiv = getSetItemData(uivItem);
        ir_method_t *callee = uiv->value.method;
        IRMETHOD_translateMethodToIR(callee);

        /* Ignore library calls. */
        if (!IRPROGRAM_doesMethodBelongToALibrary(callee)) {
            method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callee);

            /* Check for an info structure. */
            if (!calleeInfo) {
                fprintf(stderr, "Function pointer to %s not resolved.\n", IRMETHOD_getCompleteMethodName(callee));
                abort();
            }
        }

        /* Next function pointer uiv. */
        uivItem = uivItem->next;
    }
}
#endif


/**
 * For each abstract address in the given set, insert the given instruction
 * into the set of using instructions for that address in the given hash
 * table.  If there is no set of using instructions for a given abstract
 * address then a new set is created.  Returns JITTRUE if the instruction
 * was added to any sets.
 */
static JITBOOLEAN
addInstructionToUsingSet(ir_instruction_t *inst, abs_addr_set_t *aaSet, SmallHashTable *instTable) {
    JITBOOLEAN added = JITFALSE;
    abstract_addr_t *aa = absAddrSetFirst(aaSet);
    while (aa) {
        VSet *instSet = smallHashTableLookup(instTable, aa);

        /* Create a new set if necessary. */
        if (!instSet) {
            instSet = allocateNewSet(13);
            smallHashTableInsert(instTable, aa, instSet);
        }

        /* Add the instruction if not already there. */
        if (!vSetContains(instSet, inst)) {
            vSetInsert(instSet, inst);
            /*       PDEBUG("Original inst %u uses ", inst->ID); */
            /*       printAbstractAddress(aa, JITFALSE); */
            /*       PDEBUGB(" (%p)\n", aa); */
            added = JITTRUE;
        }

        /* Next abstract address. */
        aa = absAddrSetNext(aaSet, aa);
    }

    /* Return whether the instruction was added. */
    return added;
}


/**
 * Add a special abstract address for stdout to read and write sets of abstract
 * addresses, for use when computing sets for known calls.
 */
static void
addStdFilePointerToReadWriteSets(uiv_special_t type, abs_addr_set_t **readSet, abs_addr_set_t **writeSet, method_info_t *info) {
    uiv_t *uivStdout = newUivSpecial(type, info->uivs);
    abstract_addr_t *aaStdout = newAbstractAddress(uivStdout, 0);
    absAddrSetAppendAbsAddr(*readSet, aaStdout);
    absAddrSetAppendAbsAddr(*writeSet, aaStdout);
}


/**
 * Add special abstract addresses for standard file descriptors to read and
 * write sets of abstract addresses if they are referenced within the given
 * instruction parameter, for use when computing sets for known calls.
 */
static void
addParamStdFileDescriptorToReadWriteSets(ir_item_t *param, abs_addr_set_t **readSet, abs_addr_set_t **writeSet, method_info_t *info) {
    if (isImmIntParam(param)) {
        if (param->value.v >= 0 && param->value.v <= 2) {
            addStdFilePointerToReadWriteSets(param->value.v, readSet, writeSet, info);
        }
    } else if (isVariableParam(param)) {
        /* Could try to find the definition and check that. */
        addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDIN, readSet, writeSet, info);
        addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDOUT, readSet, writeSet, info);
        addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDERR, readSet, writeSet, info);
    } else {
        /* Surely can't be anything else? */
        abort();
    }
}


/**
 * Compute the sets of memory locations read and written by the given known
 * library call instruction.
 */
static void
computeKnownCallReadWriteSets(ir_instruction_t *inst, call_site_t *callSite, abs_addr_set_t **readSet, abs_addr_set_t **writeSet, method_info_t *info) {
    JITUINT32 cp;
    JITUINT32 numCallParams;
    /* PDEBUG("Known call inst %u\n", inst->ID); */
    switch (callSite->callType) {
        case OPEN_CALL:
        case ATOI_CALL:
        case GETENV_CALL: {
            /* Read the string (0). */
            abs_addr_set_t *paramSet = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, paramSet);
            freeAbstractAddressSet(paramSet);
            break;
        }

        case STRTOD_CALL: {
            /* Read from the strings (0, 1), potentially write to the string (1). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, param0Set);
            freeAbstractAddressSet(param0Set);
            if (IRMETHOD_getInstructionCallParameterType(inst, 1) == IROFFSET) {
                abs_addr_set_t *param1Set = getCallParamAccessedAbsAddrSet(inst, 1, info);
                absAddrSetAppendSet(*readSet, param1Set);
                absAddrSetAppendSet(*writeSet, param1Set);
                freeAbstractAddressSet(param1Set);
            }
            break;
        }

        case STRRCHR_CALL: {
            /* Read from the string (0). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, param0Set);
            freeAbstractAddressSet(param0Set);
            break;
        }

        case FREAD_CALL: {
            /* Read from and write the stream (3), write to the string (0). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param3Set = getCallParamReachableAbsAddrSet(inst, 3, info);
            absAddrSetAppendSet(*readSet, param0Set);
            absAddrSetAppendSet(*readSet, param3Set);
            absAddrSetAppendSet(*writeSet, param3Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param3Set);
            break;
        }

        case STRCPY_CALL:
        case STRCAT_CALL: {
            /* Read from the string (1), write to the string (0). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param1Set = getCallParamAccessedAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*readSet, param1Set);
            absAddrSetAppendSet(*writeSet, param0Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param1Set);
            break;
        }

        case STRPBRK_CALL:
        case STRSTR_CALL:
        case STRCMP_CALL: {
            /* Read from the strings (0, 1) */
            for (cp = 0; cp < 2; ++cp) {
                abs_addr_set_t *paramSet = getCallParamAccessedAbsAddrSet(inst, cp, info);
                absAddrSetAppendSet(*readSet, paramSet);
                freeAbstractAddressSet(paramSet);
            }
            break;
        }

        case SETBUF_CALL: {
            /* Read from the buffer (1), read and write the stream (0). */
            abs_addr_set_t *param0Set = getCallParamReachableAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param1Set = getCallParamAccessedAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*readSet, param0Set);
            absAddrSetAppendSet(*readSet, param1Set);
            absAddrSetAppendSet(*writeSet, param0Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param1Set);
            break ;
        }

        case SETJMP_CALL: {
            /* Write the buffer (0). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*writeSet, param0Set);
            freeAbstractAddressSet(param0Set);
            break;
        }

        case WRITE_CALL: {
            /* Read from the string (1), read and write to the file descriptor stream (0). */
            abs_addr_set_t *paramSet = getCallParamAccessedAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*readSet, paramSet);
            addParamStdFileDescriptorToReadWriteSets(IRMETHOD_getInstructionCallParameter(inst, 0), readSet, writeSet, info);
            freeAbstractAddressSet(paramSet);
            break;
        }

        case READ_CALL: {
            /* Write to the string (1), read and write to the file descriptor stream (0). */
            abs_addr_set_t *paramSet = getCallParamAccessedAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*writeSet, paramSet);
            addParamStdFileDescriptorToReadWriteSets(IRMETHOD_getInstructionCallParameter(inst, 0), readSet, writeSet, info);
            freeAbstractAddressSet(paramSet);
            break;
        }

        case FEOF_CALL: {
            /* Read from the stream (0). */
            abs_addr_set_t *param0Set = getCallParamReachableAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, param0Set);
            freeAbstractAddressSet(param0Set);
            break;
        }

        case CTIME_CALL: {
            /* Read from the time (0). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, param0Set);
            freeAbstractAddressSet(param0Set);
            break;
        }

        case TIME_CALL: {
            /* Write the time (0). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*writeSet, param0Set);
            freeAbstractAddressSet(param0Set);
            break;
        }

        case FGETS_CALL: {
            /* Read from the stream (2), write to the string and stream (0, 2). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param2Set = getCallParamReachableAbsAddrSet(inst, 2, info);
            absAddrSetAppendSet(*readSet, param2Set);
            absAddrSetAppendSet(*writeSet, param0Set);
            absAddrSetAppendSet(*writeSet, param2Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param2Set);
            break;
        }

        case REMOVE_CALL: {
            /* Read the string (0). */
            abs_addr_set_t *paramSet = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, paramSet);
            freeAbstractAddressSet(paramSet);
            break ;
        }

        case FGETC_CALL:
        case FCLOSE_CALL:
        case FSEEK_CALL:
        case FFLUSH_CALL:
        case REWIND_CALL: {
            /* Read and write the stream (0). */
            abs_addr_set_t *paramSet = getCallParamReachableAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, paramSet);
            absAddrSetAppendSet(*writeSet, paramSet);
            freeAbstractAddressSet(paramSet);
            break;
        }

        case SSCANF_CALL: {
            /* Read the string and format (0, 1), write to the rest (2...). */
            for (cp = 0; cp < 2; ++cp) {
                abs_addr_set_t *readParamSet = getCallParamAccessedAbsAddrSet(inst, cp, info);
                absAddrSetAppendSet(*readSet, readParamSet);
                freeAbstractAddressSet(readParamSet);
            }
            numCallParams = IRMETHOD_getInstructionCallParametersNumber(inst);
            for (cp = 2; cp < numCallParams; ++cp) {
                abs_addr_set_t *writeParamSet = getCallParamReachableAbsAddrSet(inst, cp, info);
                absAddrSetAppendSet(*writeSet, writeParamSet);
                freeAbstractAddressSet(writeParamSet);
            }
            break;
        }

        case FSCANF_CALL: {
            /* Read the stream and format (0, 1), write to the stream and the rest (0, 2...). */
            abs_addr_set_t *param0Set = getCallParamReachableAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param1Set = getCallParamAccessedAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*readSet, param0Set);
            absAddrSetAppendSet(*readSet, param1Set);
            absAddrSetAppendSet(*writeSet, param0Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param1Set);
            numCallParams = IRMETHOD_getInstructionCallParametersNumber(inst);
            for (cp = 2; cp < numCallParams; ++cp) {
                abs_addr_set_t *writeParamSet = getCallParamReachableAbsAddrSet(inst, cp, info);
                absAddrSetAppendSet(*writeSet, writeParamSet);
                freeAbstractAddressSet(writeParamSet);
            }
            break;
        }

        case SPRINTF_CALL: {
            /* Read all parameters after the string (1...), write to the string (0). */
            abs_addr_set_t *writeParamSet;
            numCallParams = IRMETHOD_getInstructionCallParametersNumber(inst);
            for (cp = 1; cp < numCallParams; ++cp) {
                abs_addr_set_t *readParamSet = getCallParamAccessedAbsAddrSet(inst, cp, info);
                absAddrSetAppendSet(*readSet, readParamSet);
                freeAbstractAddressSet(readParamSet);
            }
            writeParamSet = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*writeSet, writeParamSet);
            freeAbstractAddressSet(writeParamSet);
            break;
        }

        case FPRINTF_CALL: {
            /* Read all parameters, read and write to the stream (0). */
            numCallParams = IRMETHOD_getInstructionCallParametersNumber(inst);
            for (cp = 0; cp < numCallParams; ++cp) {
                abs_addr_set_t *readParamSet;
                if (cp == 0) {
                    readParamSet = getCallParamReachableAbsAddrSet(inst, cp, info);
                } else {
                    readParamSet = getCallParamAccessedAbsAddrSet(inst, cp, info);
                }
                absAddrSetAppendSet(*readSet, readParamSet);
                if (cp == 0) {
                    absAddrSetAppendSet(*writeSet, readParamSet);
                }
                freeAbstractAddressSet(readParamSet);
            }
            break;
        }

        case PERROR_CALL: {
            /* Read from the input string (0). Read and write to stderr. */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, param0Set);
            addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDERR, readSet, writeSet, info);
            freeAbstractAddressSet(param0Set);
            break ;
        }

        case PRINTF_CALL: {
            /* Read all parameters, read and write to stdout. */
            numCallParams = IRMETHOD_getInstructionCallParametersNumber(inst);
            for (cp = 0; cp < numCallParams; ++cp) {
                abs_addr_set_t *readParamSet = getCallParamAccessedAbsAddrSet(inst, cp, info);
                absAddrSetAppendSet(*readSet, readParamSet);
                freeAbstractAddressSet(readParamSet);
            }
            addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDOUT, readSet, writeSet, info);
            break;
        }

        case FOPEN_CALL: {
            /* Read both parameters. */
            for (cp = 0; cp < 2; ++cp) {
                abs_addr_set_t *readParamSet = getCallParamAccessedAbsAddrSet(inst, cp, info);
                absAddrSetAppendSet(*readSet, readParamSet);
                freeAbstractAddressSet(readParamSet);
            }
            break;
        }

        case FWRITE_CALL: {
            /* Read the string (0), read and write to the stream (3). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param3Set = getCallParamReachableAbsAddrSet(inst, 3, info);
            absAddrSetAppendSet(*readSet, param0Set);
            absAddrSetAppendSet(*readSet, param3Set);
            absAddrSetAppendSet(*writeSet, param3Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param3Set);
            break;
        }

        case FPUTS_CALL: {
            /* Read the string (0), read and write the stream (1). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param1Set = getCallParamReachableAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*readSet, param0Set);
            absAddrSetAppendSet(*readSet, param1Set);
            absAddrSetAppendSet(*writeSet, param1Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param1Set);
            break;
        }

        case PUTS_CALL: {
            /* Read the string (0), read and write to stdout. */
            abs_addr_set_t *paramSet = getCallParamAccessedAbsAddrSet(inst, 0, info);
            absAddrSetAppendSet(*readSet, paramSet);
            addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDOUT, readSet, writeSet, info);
            freeAbstractAddressSet(paramSet);
            break;
        }

        case FPUTC_CALL: {
            /* Read and write the stream (1). */
            abs_addr_set_t *paramSet = getCallParamReachableAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*readSet, paramSet);
            absAddrSetAppendSet(*writeSet, paramSet);
            freeAbstractAddressSet(paramSet);
            break;
        }

        case PUTCHAR_CALL: {
            /* Read and write to stdout. */
            addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDOUT, readSet, writeSet, info);
            break;
        }

        case QSORT_CALL: {
            /* Read and write to the base (0), read the function pointer (3). */
            abs_addr_set_t *param0Set = getCallParamAccessedAbsAddrSet(inst, 0, info);
            abs_addr_set_t *param3Set = getCallParamAccessedAbsAddrSet(inst, 3, info);
            absAddrSetAppendSet(*readSet, param0Set);
            absAddrSetAppendSet(*readSet, param3Set);
            absAddrSetAppendSet(*writeSet, param0Set);
            freeAbstractAddressSet(param0Set);
            freeAbstractAddressSet(param3Set);
            break;
        }

        case BUFFER_WRITEBITS_CALL: {
            /* Read the bits stored in (1), read and write to stdout. */
            abs_addr_set_t *paramSet = getCallParamAccessedAbsAddrSet(inst, 1, info);
            absAddrSetAppendSet(*readSet, paramSet);
            addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDOUT, readSet, writeSet, info);
            freeAbstractAddressSet(paramSet);
            break;
        }

        case BUFFER_READBITS:{
            /* Read and write to stdout. */
            addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDOUT, readSet, writeSet, info);
            break;
        }

        case BUFFER_FLUSH_CALL: {
            addStdFilePointerToReadWriteSets(UIV_SPECIAL_STDOUT, readSet, writeSet, info);
            break;
        }

        case BUFFER_WRITEBITSFROMVALUE_CALL:
        case BUFFER_SETSTREAM:
        case BUFFER_SETNUMBEROFBYTESWRITTEN:
        case BUFFER_GETNUMBEROFBYTESWRITTEN:
        case BUFFER_NEEDTOREADNUMBEROFBITS:
        case ERRNO_CALL:
        case STRERROR_CALL:
        case TMPFILE_CALL:
        case DIFFTIME_CALL:
        case FILENO_CALL:
        case MAXMIN_CALL:
        case ABS_CALL:
        case RAND_CALL:
        case TOLOWER_CALL:
        case TOUPPER_CALL:
        case ISUPPER_CALL:
        case ISALPHA_CALL:
        case ISALNUM_CALL:
        case ISSPACE_CALL:
        case EXIT_CALL:
        case CLOSE_CALL:
        case CLOCK_CALL:
        case CALLOC_CALL:
        case MALLOC_CALL:
        case IO_FTABLE_GET_ENTRY_CALL:
            /* Don't read or write anything. */
            break;

        default:
            abort();
    }
}


/**
 * Compute the read a write sets of abstract address from one call site.
 */
static void
computeReadWriteSetsFromCallSite(ir_instruction_t *callInst, call_site_t *callSite, abs_addr_set_t **callReadSet, abs_addr_set_t **callWriteSet, method_info_t *callerInfo, SmallHashTable *methodsInfo) {
    /* Normal calls. */
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        SmallHashTable *uivMap = smallHashTableLookup(callerInfo->calleeUivMaps, callSite->method);
        abs_addr_set_t *callerSet;

        /* Debugging. */
        /* PDEBUG("  Read / write sets for %s\n", IRMETHOD_getMethodName(calleeInfo->ssaMethod)); */
        /* PDEBUG("  Read set is: "); */
        /* printAbstractAddressSet(calleeInfo->readSet); */
        /* PDEBUG("  Write set is: "); */
        /* printAbstractAddressSet(calleeInfo->writeSet); */

        /* Add read locations. */

        /* TODO: Cache this information. */
        /* abort(); */

        callerSet = mapCalleeAbsAddrSetToCallerAbsAddrSet(calleeInfo->readSet, callerInfo, calleeInfo, uivMap);
        assert(callerSet);
        absAddrSetAppendSet(*callReadSet, callerSet);
        freeAbstractAddressSet(callerSet);

        /* Add write locations. */
        callerSet = mapCalleeAbsAddrSetToCallerAbsAddrSet(calleeInfo->writeSet, callerInfo, calleeInfo, uivMap);
        assert(callerSet);
        absAddrSetAppendSet(*callWriteSet, callerSet);
        freeAbstractAddressSet(callerSet);
    }

    /* Known library calls. */
    else if (callSite->callType != LIBRARY_CALL) {
        computeKnownCallReadWriteSets(callInst, callSite, callReadSet, callWriteSet, callerInfo);
    }
}


/**
 * Compute the sets of memory locations read and written by the given
 * method.  Returns JITTRUE if there were any changes to any of the sets.
 */
static JITBOOLEAN
computeReadWriteSetsInMethod(method_info_t *info, SmallHashTable *methodsInfo) {
    JITBOOLEAN changes = JITFALSE;
    JITUINT32 i;
    abs_addr_set_t *originalReadSet;
    abs_addr_set_t *originalWriteSet;

    /* The original read and write sets. */
    originalReadSet = absAddrSetClone(info->readSet);
    originalWriteSet = absAddrSetClone(info->writeSet);

    /* Add to each set depending on instruction type. */
    PDEBUG("Computing read / write sets for %s\n", IRMETHOD_getMethodName(info->ssaMethod));
    for (i = 0; i < IRMETHOD_getInstructionsNumber(info->ssaMethod); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        abs_addr_set_t *varSet;
        switch (inst->type) {
            case IRLOADREL:

                /* /\** */
                /*  * Need to add support for loading multiple memory addresses based on the */
                /*  * length of the destination type.  Otherwise this could miss addresses. */
                /*  *\/ */
                /* abort(); */

                /* Add read locations. */
                varSet = getLoadStoreAccessedAbsAddrSet(inst, info);
                absAddrSetAppendSet(info->readSet, varSet);
                freeAbstractAddressSet(varSet);
                break;

            case IRSTOREREL:

                /* Add write locations. */
                varSet = getLoadStoreAccessedAbsAddrSet(inst, info);
                absAddrSetAppendSet(info->writeSet, varSet);
                freeAbstractAddressSet(varSet);
                break;

            case IRCALL:
            case IRICALL:
            case IRVCALL: {

                /* Set up the call read and write set for this instruction. */
                SmallHashTableItem *callReadItem = smallHashTableLookupItem(info->callReadMap, inst);
                SmallHashTableItem *callWriteItem = smallHashTableLookupItem(info->callWriteMap, inst);
                abs_addr_set_t **callReadSet = (abs_addr_set_t **)&(callReadItem->element);
                abs_addr_set_t **callWriteSet = (abs_addr_set_t **)&(callWriteItem->element);
                void *callTarget = smallHashTableLookup(info->callMethodMap, inst);
                assert(callReadSet && callWriteSet);
                /* PDEBUG("Merging call read and write sets for inst %u\n", inst->ID); */

                /* Work through each call site. */
                if (inst->type == IRCALL) {
                    computeReadWriteSetsFromCallSite(inst, callTarget, callReadSet, callWriteSet, info, methodsInfo);
                } else {
                    XanList *callSiteList = (XanList *)callTarget;
                    XanListItem *callSiteItem = xanList_first(callSiteList);
                    while (callSiteItem) {
                        call_site_t *callSite = callSiteItem->data;
                        computeReadWriteSetsFromCallSite(inst, callSite, callReadSet, callWriteSet, info, methodsInfo);
                        callSiteItem = callSiteItem->next;
                    }
                }

                /* Merge the read and write sets for this instruction. */
                absAddrSetMerge(*callReadSet, info->uivMergeTargets);
                absAddrSetMerge(*callWriteSet, info->uivMergeTargets);

                /* Append read and write sets to those for the method. */
                absAddrSetAppendSet(info->readSet, *callReadSet);
                absAddrSetAppendSet(info->writeSet, *callWriteSet);

                /* Debugging. */
                /* PDEBUG("  Call read set is: "); */
                /* printAbstractAddressSet(*callReadSet); */
                /* PDEBUG("  Call write set is: "); */
                /* printAbstractAddressSet(*callWriteSet); */
                break;
            }
        }
    }

    /* Merge the read and write sets for this method. */
    absAddrSetMerge(info->readSet, info->uivMergeTargets);
    absAddrSetMerge(info->writeSet, info->uivMergeTargets);

    /* Determine whether changes have occurred. */
    changes |= !abstractAddressSetsAreIdentical(info->readSet, originalReadSet);
    changes |= !abstractAddressSetsAreIdentical(info->writeSet, originalWriteSet);

    /* Free space. */
    freeAbstractAddressSet(originalReadSet);
    freeAbstractAddressSet(originalWriteSet);

    /* Return whether there were changes. */
    return changes;
}


/**
 * Apply a merge to the given abstract address set and then record the
 * given instruction as a user of each abstract address in the merged set.
 */
static void
assignInstToMergedAbsAddrSet(ir_instruction_t *inst, SmallHashTable *mergeMap, abs_addr_set_t *aaSet, SmallHashTable *instMap) {
    applyGenericMergeMapToAbstractAddressSet(aaSet, mergeMap);
    addInstructionToUsingSet(inst, aaSet, instMap);
}


/**
 * Get the abstract address set for the given variable, apply a merger to
 * it and then record the given instruction as a user of each abstract
 * address.
 */
static void
assignInstToMergedVarAbsAddrSet(ir_instruction_t *inst, method_info_t *info, SmallHashTable *instMap) {
    abs_addr_set_t *unmergedSet = getVarAbstractAddressSet(IRMETHOD_getInstructionParameter1(inst), info, JITFALSE);
    abs_addr_set_t *mergedSet = absAddrSetClone(unmergedSet);
    assignInstToMergedAbsAddrSet(inst, info->mergeAbsAddrMap, mergedSet, instMap);
    freeAbstractAddressSet(mergedSet);
}


/**
 * Assign a call instruction to the abstract addresses that it reads and writes
 * through a single call site.
 */
static void
assignInstToAbsAddrUsingSetForCallSite(ir_instruction_t *callInst, call_site_t *callSite, method_info_t *callerInfo, SmallHashTable *methodsInfo) {
    /* Ignore library calls. */
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        SmallHashTable *uivMap = smallHashTableLookup(callerInfo->calleeUivMaps, callSite->method);
        abs_addr_set_t *callerSet;
        /* TODO: Cache this information. */
        /* abort(); */

        /* Add read locations. */
        callerSet = mapCalleeAbsAddrSetToCallerAbsAddrSet(calleeInfo->readSet, callerInfo, calleeInfo, uivMap);
        assert(callerSet);
        assignInstToMergedAbsAddrSet(callInst, callerInfo->mergeAbsAddrMap, callerSet, callerInfo->readInsts);
        freeAbstractAddressSet(callerSet);

        /* Add write locations. */
        callerSet = mapCalleeAbsAddrSetToCallerAbsAddrSet(calleeInfo->writeSet, callerInfo, calleeInfo, uivMap);
        assert(callerSet);
        assignInstToMergedAbsAddrSet(callInst, callerInfo->mergeAbsAddrMap, callerSet, callerInfo->writeInsts);
        freeAbstractAddressSet(callerSet);
    }
}


/**
 * For each memory location read and written by the given method, create a
 * mapping to the instructions that do the reading or writing.  This
 * information can then be used in computing dependences between
 * instructions in a subsequent pass, without needing to recompute it each
 * time a call to the method is encountered.
 */
static void
assignInstsToAbsAddrUsingSets(method_info_t *info, SmallHashTable *methodsInfo) {
    JITUINT32 i;

    /* Add to each set depending on instruction type. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(info->ssaMethod); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        switch (inst->type) {
            case IRLOADREL:
                assignInstToMergedVarAbsAddrSet(inst, info, info->readInsts);
                break;

            case IRSTOREREL:
                assignInstToMergedVarAbsAddrSet(inst, info, info->writeInsts);
                break;

            case IRCALL: {
                call_site_t *callSite = smallHashTableLookup(info->callMethodMap, inst);
                assignInstToAbsAddrUsingSetForCallSite(inst, callSite, info, methodsInfo);
                break;
            }

            case IRICALL:
            case IRVCALL: {

                /* Set up the call read and write set for this instruction. */
                XanList *callSiteList = smallHashTableLookup(info->callMethodMap, inst);

                /* Work through each call site. */
                if (xanList_length(callSiteList) > 0) {
                    XanListItem *callSiteItem = xanList_first(callSiteList);
                    while (callSiteItem) {
                        call_site_t *callSite = callSiteItem->data;
                        assignInstToAbsAddrUsingSetForCallSite(inst, callSite, info, methodsInfo);
                        callSiteItem = callSiteItem->next;
                    }
                }

                /* No call sites. */
                else {
                    addInstructionToUsingSet(inst, info->readSet, info->readInsts);
                    addInstructionToUsingSet(inst, info->writeSet, info->writeInsts);
                }
                break;
            }

            case IRLIBRARYCALL:
            case IRNATIVECALL:

                /* Add to all read and write locations. */
                if (IRMETHOD_mayInstructionAccessHeapMemory(info->ssaMethod, inst)) {
                    addInstructionToUsingSet(inst, info->readSet, info->readInsts);
                    addInstructionToUsingSet(inst, info->writeSet, info->writeInsts);
                }
                break;
        }
    }
}


/**
 * Compute the read and write sets for each method within an SCC.
 */
static void
computeReadWriteSetsInSCC(XanList *scc, SmallHashTable *methodsInfo, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap) {
    JITBOOLEAN changes = JITTRUE;
    XanListItem *currMethod;
    FIXED_POINT_ITER_START;

    /* Compute the read and write sets for each method. */
    while (changes) {
        FIXED_POINT_ITER_NEW(MAX(20, 2 * xanList_length(scc)), "computing aliases");

        /* Reset termination condition. */
        changes = JITFALSE;

        /* Analyse all methods in the SCC. */
        currMethod = xanList_first(scc);
        while (currMethod) {
            ir_method_t *method = currMethod->data;
            method_info_t *info = smallHashTableLookup(methodsInfo, method);
            GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug);
            if (enableExtendedDebugPrinting(method)) {
                SET_DEBUG_ENABLE(JITTRUE);
            }
            changes |= computeReadWriteSetsInMethod(info, methodsInfo);
            SET_DEBUG_ENABLE(prevEnablePrintDebug);
            currMethod = xanList_next(currMethod);
        }
    }

    /* Fixed point reached. */
    FIXED_POINT_ITER_END;

    /* Extend each read and write set with concrete values. */
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        addConcreteValuesToAbsAddrSet(info->readSet, info, concreteValues, concreteUivToKeyMap);
        addConcreteValuesToAbsAddrSet(info->writeSet, info, concreteValues, concreteUivToKeyMap);
        currMethod = xanList_next(currMethod);
        PDEBUGNL("Read / write sets for %s\n", IRMETHOD_getMethodName(info->ssaMethod));
        PDEBUG("Read set is: ");
        printAbstractAddressSet(info->readSet);
        PDEBUG("Write set is: ");
        printAbstractAddressSet(info->writeSet);
    }
}


/**
 * Compute the sets of instructions that use each abstract address for
 * each method within an SCC.
 */
static void
computeInstsUsingAbsAddrsInSCC(XanList *scc, SmallHashTable *methodsInfo) {
    XanListItem *currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        assignInstsToAbsAddrUsingSets(info, methodsInfo);
        currMethod = xanList_next(currMethod);
    }
}


/**
 * Determine the definitions of each load and store memory base variable
 * used within the given method.
 */
static void
determineMemoryBaseDefinitionsInMethod(ir_method_t *origMethod, method_info_t *info, SmallHashTable *methodsInfo) {
    JITUINT32 i;

    /* Find loads and stores to memory. */
    for (i = 0; i< IRMETHOD_getInstructionsNumber(info->ssaMethod); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        ir_instruction_t *origInst;
        abs_addr_set_t *varSet;
        abstract_addr_t *aa;

        /* Find the right instructions. */
        switch (inst->type) {
            case IRLOADREL:
            case IRSTOREREL:

                /* Find definitions for each base abstract address. */
                origInst = smallHashTableLookup(info->instMap, inst);
                varSet = getLoadStoreAccessedAbsAddrSet(inst, info);
                aa = absAddrSetFirst(varSet);
                while (aa) {
                    uiv_t *baseUiv = getBaseUiv(aa->uiv);

                    /* Allocated memory is interesting. */
                    if (baseUiv->type == UIV_ALLOC) {
                        VSet *ssaDefSet = smallHashTableLookup(info->uivDefMap, baseUiv);
                        XanList *origDefList = allocateNewList();
                        VSetItem *instItem;
                        assert(ssaDefSet);
                        assert(vSetSize(ssaDefSet) > 0);

                        /* Translate instructions to original method's. */
                        instItem = vSetFirst(ssaDefSet);
                        while (instItem) {
                            ir_instruction_t *ssaInst = getSetItemData(instItem);
                            ir_instruction_t *origDef = smallHashTableLookup(info->instMap, ssaInst);
                            assert(origDef);
                            xanList_append(origDefList, origDef);
                            instItem = vSetNext(ssaDefSet, instItem);
                        }

                        /* Record this set. */
                        IRMETHOD_setInstructionMetadata(origInst, DDG_COMPUTER, origDefList);

                        /* This list cannot be freed. */
                        willNotFreeList(origDefList);
                    }

                    /** We don't know what a parameter is, so include that too.  Must
                     * include a check on it being a variable parameter, since it could
                     * come from a get_address instruction.
                     */
                    else if (baseUiv->type == UIV_VAR && IRMETHOD_isTheVariableAMethodParameter(info->ssaMethod, getVarFromBaseUiv(baseUiv))) {
                        XanList *origDefList = allocateNewList();
                        ir_instruction_t *origDef = IRMETHOD_getInstructionAtPosition(origMethod, getVarFromBaseUiv(baseUiv));
                        assert(origDef);
                        xanList_append(origDefList, origDef);
                        IRMETHOD_setInstructionMetadata(origInst, DDG_COMPUTER, origDefList);

                        /* This list cannot be freed. */
                        willNotFreeList(origDefList);
                    }

                    /* Next abstract address. */
                    aa = absAddrSetNext(varSet, aa);
                }
                break;
        }
    }
}


/**
 * An extra phase of the algorithm to ease parallelisation and help reduce
 * unnecessary synchronisation.  We consider the base variables used in
 * memory load and store and work out the instructions that define the
 * memory pointed to by that variable.
 */
static void
determineMemoryBaseDefinitionsInSCC(XanList *scc, SmallHashTable *methodsInfo) {
    XanListItem *currMethod;
    if (!provideMemoryAllocators) {
        return;
    }
    currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        determineMemoryBaseDefinitionsInMethod(method, info, methodsInfo);
        currMethod = xanList_next(currMethod);
    }
}


/**
 * Propagate concrete pointer values from caller to callee across a call site.
 */
static void
propagateConcretePointersAcrossCallSite(call_site_t *callSite, method_info_t *callerInfo, SmallHashTable *methodsInfo, VSet *methodsCanBeProcessed) {
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        SmallHashTableItem *mapItem = smallHashTableFirst(callerInfo->memAbsAddrMap);
        while (mapItem) {
            aa_mapped_set_t *mappedSet = mapItem->element;
            appendSetToMemAbsAddrSet(mapItem->key, calleeInfo, mappedSet->aaSet);
            mapItem = smallHashTableNext(callerInfo->memAbsAddrMap, mapItem);
        }
        vSetInsert(methodsCanBeProcessed, callSite->method);
    }
}


/**
 * Propagate concrete pointer values from a single method to its callees.
 * Basically everything in memory will be propagated.
 */
static void
propagateConcretePointersFromMethod(method_info_t *info, SmallHashTable *methodsInfo, VSet *methodsCanBeProcessed) {
    SmallHashTableItem *callItem;

    /* Work through each callee in order. */
    callItem = smallHashTableFirst(info->callMethodMap);
    while (callItem) {
        if (((ir_instruction_t *)callItem->key)->type == IRCALL) {
            propagateConcretePointersAcrossCallSite(callItem->element, info, methodsInfo, methodsCanBeProcessed);
        } else {
            XanList *callSiteList = callItem->element;
            XanListItem *callSiteItem = xanList_first(callSiteList);
            while (callSiteItem) {
                call_site_t *callSite = callSiteItem->data;
                propagateConcretePointersAcrossCallSite(callSite, info, methodsInfo, methodsCanBeProcessed);
                callSiteItem = callSiteItem->next;
            }
        }
        callItem = smallHashTableNext(info->callMethodMap, callItem);
    }
}


/**
 * Propagate concrete pointer values into a single method.
 */
static void
propagateConcretePointersToMethod(method_info_t *info, SmallHashTable *methodsInfo, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap) {
    JITUINT32 i, numInsts = IRMETHOD_getInstructionsNumber(info->ssaMethod);
    XanBitSet *alteredVars = allocateNewBitSet(IRMETHOD_getNumberOfVariablesNeededByMethod(info->ssaMethod));

    /* Debugging. */
    PDEBUG("Propagating concrete pointers for %s\n", IRMETHOD_getMethodName(info->ssaMethod));

    /* Check each instruction. */
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);

        /* Should use a switch here to deal with other memory instructions. */
        if (inst->type == IRLOADREL || inst->type == IRSTOREREL) {
            ir_item_t *baseParam = IRMETHOD_getInstructionParameter1(inst);
            if (baseParam->type == IROFFSET) {
                abs_addr_set_t *varSet = getVarAbstractAddressSet(baseParam, info, JITFALSE);
                if (!absAddrSetIsEmpty(varSet)) {


                }
            }
        }
    }
}


/**
 * Propagate concrete pointer values from methods in the SCC to each of their
 * callees.  I take this to mean that all abstract addresses in memory need to
 * be transferred if they are based on a global or allocation uiv.
 */
static void
propagateConcretePointersFromSCC(XanList *scc, SmallHashTable *methodsInfo, VSet *methodsCanBeProcessed, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap) {
    XanList *methodList;

    /* Copy the list to methods can be removed. */
    methodList = xanList_cloneList(scc);
    while (xanList_length(methodList) > 0) {
        XanListItem *currMethod;
        JITBOOLEAN found = JITFALSE;

        /* Choose methods that can be processed. */
        currMethod = xanList_first(methodList);
        while (currMethod) {
            XanListItem *nextMethod = currMethod->next;
            ir_method_t *method = currMethod->data;
            /* if (vSetContains(methodsCanBeProcessed, method)) { */
                method_info_t *info = smallHashTableLookup(methodsInfo, method);
                propagateConcretePointersToMethod(info, methodsInfo, concreteValues, concreteUivToKeyMap);
                xanList_deleteItem(methodList, currMethod);
                found = JITTRUE;
            /* } */
            currMethod = nextMethod;
        }

        /* If this fails, something bad is wrong with the SCC. */
        if (!found) {
            abort();
        }
    }

    /* Clean up. */
    freeListUnallocated(methodList);
}


/**
 * Create a mapping between abstract addresses and their concrete values.  This
 * means, for us, for each memory set within the entry method creating the
 * abstract address that would be created by a load to that location, then
 * removing this from the mapped-to set and, if that set is now non-empty,
 * storing that abstract address as being equivalent to the set.
 */
static SmallHashTable *
createConcretePointerValueTable(SmallHashTable *methodsInfo) {
    ir_method_t *method = IRPROGRAM_getEntryPointMethod();
    method_info_t *info = smallHashTableLookup(methodsInfo, method);
    SmallHashTable *concreteValues = allocateNewHashTable();
    SmallHashTableItem *mapItem = smallHashTableFirst(info->memAbsAddrMap);
    while (mapItem) {
        abstract_addr_t *aaKey = mapItem->key;
        abstract_addr_t *aaValue = newAbstractAddress(newUivFromUiv(aaKey->uiv, aaKey->offset, info->uivs), 0);
        aa_mapped_set_t *mappedSet = mapItem->element;
        abs_addr_set_t *aaSet = absAddrSetCloneWithoutAbsAddr(mappedSet->aaSet, aaValue);
        if (!absAddrSetIsEmpty(aaSet)) {
            assert(!smallHashTableContains(concreteValues, aaValue));
            smallHashTableInsert(concreteValues, aaValue, aaSet);
        } else {
            freeAbstractAddressSet(aaSet);
        }
        mapItem = smallHashTableNext(info->memAbsAddrMap, mapItem);
    }

    /* Debugging. */
    PDEBUGNL("Concrete value map\n");
    mapItem = smallHashTableFirst(concreteValues);
    while (mapItem) {
        PDEBUG("");
        printAbstractAddress(mapItem->key, JITFALSE);
        PDEBUGB(" = ");
        printAbstractAddressSet(mapItem->element);
        mapItem = smallHashTableNext(concreteValues, mapItem);
    }
    return concreteValues;
}


/**
 * Create a mapping between base uivs and the key abstract addresses within the
 * given table.
 */
static SmallHashTable *
createUivToKeyAbsAddrListMap(SmallHashTable *absAddrMap) {
    SmallHashTable *uivToKeyMap = allocateNewHashTable();
    SmallHashTableItem *mapItem = smallHashTableFirst(absAddrMap);
    while (mapItem) {
        abstract_addr_t *aaKey = mapItem->key;
        uiv_t *baseUiv = getBaseUiv(aaKey->uiv);
        XanList *aaList = smallHashTableLookup(uivToKeyMap, baseUiv);
        if (!aaList) {
            aaList = allocateNewList();
            smallHashTableInsert(uivToKeyMap, baseUiv, aaList);
        }
        xanList_append(aaList, aaKey);
        mapItem = smallHashTableNext(absAddrMap, mapItem);
    }

    /* Debugging. */
    PDEBUGNL("Concrete value uiv to key map\n");
    mapItem = smallHashTableFirst(uivToKeyMap);
    while (mapItem) {
        uiv_t *baseUiv = mapItem->key;
        XanList *aaList = mapItem->element;
        PDEBUG("");
        printUiv(baseUiv, "", JITFALSE);
        PDEBUGB(": ");
        printAbstractAddressList(aaList);
        mapItem = smallHashTableNext(uivToKeyMap, mapItem);
    }
    return uivToKeyMap;
}


/**
 * Perform phase 3 of the algorithm which is computing aliases.  Actually,
 * this computes the memory dependences between instructions using the
 * information computed by this pass.  Since we're trying to minimise the
 * amount of memory used by the pass, we do everything for each method and
 * throw away all information as soon as possible.  Therefore we have to
 * do the analysis first each time.
 */
static void
performFinalAnalysisAndComputeAliases(XanList *allSccs, SmallHashTable *methodsInfo) {
    XanListItem *currScc;
    VSet *methodsCanBeProcessed;
    SmallHashTable *concreteValues;
    SmallHashTable *concreteUivToKeyMap;

    /* Debugging. */
    PDEBUGNL("************************************************************\n");
    PDEBUGLITE("Performing Phase 3: Computing final analysis and aliases\n");

    /* The mappings between abstract addresses and concrete values. */
    concreteValues = createConcretePointerValueTable(methodsInfo);
    concreteUivToKeyMap = createUivToKeyAbsAddrListMap(concreteValues);

    /* A set of methods that can be processed, for SCC ordering. */
    methodsCanBeProcessed = allocateNewSet(30);
    vSetInsert(methodsCanBeProcessed, IRPROGRAM_getEntryPointMethod());

    /* Traverse in topological order one SCC at a time. */
    /* fprintf(stderr, "Performing Phase 3: Computing final analysis and aliases\n"); */
    /* currScc = xanList_last(allSccs); */
    /* while (currScc) { */
    /*     XanList *scc = currScc->data; */
    /*     propagateConcretePointersFromSCC(scc, methodsInfo, methodsCanBeProcessed, concreteValues, concreteUivToKeyMap); */
    /*     currScc = currScc->prev; */
    /* } */

    /**
     * We should be more intelligent about this.  We don't really want to run
     * all the analysis again so we should transfer memory pointers as is done
     * above and then inspect the variable sets of abstract addresses,
     * substituting pointers where we can.  We should also make sure that we
     * don't transfer too much.  If a method can never be a caller (direct or
     * indirect) of a method, then its variables should never propagate up
     * to this method.
     **/

    /* Now compute aliases, this time in reverse-topological order. */
    currScc = xanList_first(allSccs);
    while (currScc) {
        XanList *scc = currScc->data;
        /* performAnalysisInSCC(scc, methodsInfo); */
        computeReadWriteSetsInSCC(scc, methodsInfo, concreteValues, concreteUivToKeyMap);
        if (computeOutsideInsts) {
            computeInstsUsingAbsAddrsInSCC(scc, methodsInfo);
        }
        computeMemoryDependencesInSCC(scc, methodsInfo, concreteValues, concreteUivToKeyMap);
        determineMemoryBaseDefinitionsInSCC(scc, methodsInfo);
        clearCalleeMethodsInfosInSCC(scc, methodsInfo);
        currScc = currScc->next;
    }

    /* Clean up. */
    freeListHashTable(concreteUivToKeyMap);
    freeAbsAddrSetHashTable(concreteValues);
    freeSet(methodsCanBeProcessed);
}


/**
 * Get and print basic statistics about the program to be analysed.
 */
static void
printProgramStats(ir_method_t *startMethod) {
    if (printProgramStatistics) {
        XanList *reachableMethods;
        XanListItem *currMethod;
        JITUINT32 numMethods = 0;
        JITUINT32 numIcalls = 0;
        JITUINT32 numOperations = 0;

        /* Get all methods reachable from this. */
        reachableMethods = IRPROGRAM_getReachableMethods(startMethod);
        currMethod = xanList_first(reachableMethods);
        while (currMethod) {
            ir_method_t *method = currMethod->data;
            if (!IRPROGRAM_doesMethodBelongToALibrary(method)) {
                JITUINT32 i;
                JITUINT32 numInsts = IRMETHOD_getInstructionsNumber(method);
                for (i = 0; i < numInsts; ++i) {
                    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
                    switch (inst->type) {
                        case IRICALL:
                            numIcalls += 1;
                            break;
                        case IRADD:
                        case IRADDOVF:
                        case IRSUB:
                        case IRSUBOVF:
                        case IRMUL:
                        case IRMULOVF:
                        case IRDIV:
                        case IRREM:
                        case IRAND:
                        case IRNEG:
                        case IROR:
                        case IRNOT:
                        case IRXOR:
                        case IRSHL:
                        case IRSHR:
                        case IRLT:
                        case IRGT:
                        case IREQ:
                        case IRLOADREL:
                        case IRSTOREREL:
                        case IRMEMCPY:
                        case IRMEMCMP:
                        case IRSTRINGCMP:
                        case IRCOSH:
                        case IRCEIL:
                        case IRSIN:
                        case IRCOS:
                        case IRACOS:
                        case IRSQRT:
                        case IRFLOOR:
                        case IRPOW:
                        case IREXP:
                        case IRLOG10:
                            numOperations += 1;
                    }
                }
                numMethods += 1;
            }
            currMethod = currMethod->next;
        }

        /* Print stuff. */
        fprintf(stderr, "Methods:        %4u\n", numMethods);
        fprintf(stderr, "Operations:     %4u\n", numOperations);
        fprintf(stderr, "Indirect calls: %4u\n", numIcalls);

        /* Clean up. */
        xanList_destroyList(reachableMethods);
    }
}


/**
 * Initialise global constants based on environment variables.
 */
static inline void
initEnvironment(void) {
    char *env;
    env = getenv("DDG_PRINT_PROGRAM_STATS");
    if (env) {
        printProgramStatistics = atoi(env) != 0;
    }
    env = getenv("DDG_IP_USE_KNOWN_CALLS");
    if (env && atoi(env) == 0) {
        useKnownCalls = JITFALSE;
    }
    env = getenv("DDG_IP_USE_TYPE_INFOS");
    if (env && atoi(env) == 0) {
        useTypeInfos = JITFALSE;
    }
    env = getenv("DDG_IP_CALCULATE_FUNCTION_POINTERS");
    if (env) {
        calculateFunctionPointers = atoi(env) == 1;
    }
    env = getenv("DDG_IP_EXTENDED_PRINTDEBUG");
    if (env) {
        extendedPrintDebugFunctionName = env;
    }
}


/**
 * Entry point to the VLLPA algorithm.  Assumes that the given method is
 * the main method or method to use as the start point for the call graph.
 */
void doVllpa(ir_method_t *method) {
    ir_method_t *entryMethod;
    JITBOOLEAN hasFuncPtrs;
    JITBOOLEAN resolvedFuncPtrs;
    SmallHashTable *methodsInfo;
    XanList *allSccs = NULL;
    XanListItem *sccItem;
    JITUINT32 iter = 0;
#ifdef PRINTDEBUG
    char *env;
    char *prevName = getenv("DOTPRINTER_FILENAME");
    GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePDebug);
#endif
    newTimer(vllpaStartTime);
    newTimer(vllpaEndTime);
    newTimer(startAliasTime);
    newTimer(startCleanupTime);

    /* Get environment variables. */
    setTimer(&vllpaStartTime);
    startInstrumentation();
    initEnvironment();

    /* Initialise memory and types used. */
    smallHashTableInit();
    vllpaInitMemory();
    vllpaInitTypes();

    /* Initialise globals. */
    initGlobals();

    /**
     * Outside instructions don't work right now.  We need to handle known calls
     * and check that the abstract address set prefixes checked are correct when
     * calculating callee instructions that cause the dependences.
     */
    if (computeOutsideInsts) {
        abort();
    }

    /* Debugging output. */
#ifdef PRINTDEBUG
    env = getenv("VLLPA_DEBUG_LEVEL");
    if (env && atoi(env) > 0) {
        SET_DEBUG_ENABLE(JITTRUE);
    } else {
        SET_DEBUG_ENABLE(JITFALSE);
    }
    setenv("DOTPRINTER_FILENAME", "VLLPA", 1);
#endif
    PDEBUGLITE("Starting VLLPA\n");

    /* We want to do the analysis on all methods. */
    entryMethod = IRPROGRAM_getEntryPointMethod();

    /* Get statistics about the program to be analysed. */
    printProgramStats(entryMethod);

    /* Initialise the hash table containing information about each method. */
    methodsInfo = initialiseInfo(entryMethod);

    /* Check whether we should skip resolving function pointers. */
    /*   hasFuncPtrs = JITFALSE; */
    hasFuncPtrs = programHasIndirectCalls(methodsInfo);
    PDEBUGLITE("Program has %sfunction pointers\n", hasFuncPtrs ? "" : "no ");

    /* Iterate while function pointers are resolved. */
    resolvedFuncPtrs = JITTRUE;
    while (calculateFunctionPointers && resolvedFuncPtrs) {
        newTimer(startSccTime);
        newTimer(startAnalysisTime);
        newTimer(startFuncPtrTime);
        newTimer(endIterTime);
        PDEBUG("Iteration %u of whole algorithm\n", iter);
        ++iter;
        if (iter > 10) {
            abort();
        }

        /* Phase 0: Build call graph. */
        setTimer(&startSccTime);
        allSccs = getStronglyConnectedComponents(entryMethod, methodsInfo);

        /* Phase 1: Intraprocedural and interprocedural analyses. */
        setTimer(&startAnalysisTime);
        performAnalysis(allSccs, methodsInfo);

        /* Phase 2: Propagate concrete function names. */
        setTimer(&startFuncPtrTime);
        if (hasFuncPtrs) {
            resolvedFuncPtrs = linkICallsToTargets(allSccs, methodsInfo);
        } else {
            resolvedFuncPtrs = JITFALSE;
        }

        /* Debugging output. */
        if (!resolvedFuncPtrs) {
            PDEBUGLITE("No function pointers resolved\n");
            /*       checkFunctionPointersResolved(method, methodsInfo); */
        }

        /* Clean up for next iteration. */
        else {
            clearAllInfoMappings(methodsInfo);
            sccItem = xanList_first(allSccs);
            while (sccItem) {
                XanList *scc = sccItem->data;
                freeList(scc);
                sccItem = sccItem->next;
            }
            freeList(allSccs);
        }

        /* Work out time taken. */
        setTimer(&endIterTime);
        updateTiming(sccTime, startSccTime, startAnalysisTime);
        updateTiming(analysisTime, startAnalysisTime, startFuncPtrTime);
        updateTiming(funcPtrTime, startFuncPtrTime, endIterTime);
    }

    /* Phase 3: Propagate concrete pointer values and compute aliases. */
    setTimer(&startAliasTime);
    allSccs = getStronglyConnectedComponents(entryMethod, methodsInfo);
    performFinalAnalysisAndComputeAliases(allSccs, methodsInfo);
    dumpInstrumentation();
    stopInstrumentation();

    /* Clean up. */
    setTimer(&startCleanupTime);
    sccItem = xanList_first(allSccs);
    while (sccItem) {
        XanList *scc = sccItem->data;
        freeList(scc);
        sccItem = sccItem->next;
    }
    freeList(allSccs);
    freeInfo(methodsInfo);

    /* Reset environment variable. */
#ifdef PRINTDEBUG
    SET_DEBUG_ENABLE(prevEnablePDebug);
    setenv("DOTPRINTER_FILENAME", prevName, 1);
#endif

    /* Report the time taken. */
#ifdef DEBUG_TIMINGS
    setTimer(&vllpaEndTime);
    updateTiming(aliasTime, startAliasTime, startCleanupTime);
    updateTiming(vllpaTime, vllpaStartTime, vllpaEndTime);
    fprintf(stderr, "Analysis time:     %7.3lf\n", 1.0 * analysisTime / 1000000.0);
    fprintf(stderr, "  Points to time:  %7.3lf\n", 1.0 * pointsToTime / 1000000.0);
    fprintf(stderr, "  Transfer time:   %7.3lf\n", 1.0 * transferTime / 1000000.0);
    fprintf(stderr, "Func ptr time:     %7.3lf\n", 1.0 * funcPtrTime / 1000000.0);
    fprintf(stderr, "Alias time:        %7.3lf\n", 1.0 * aliasTime / 1000000.0);
    fprintf(stderr, "Total VLLPA time:  %7.3lf\n", 1.0 * vllpaTime / 1000000.0);
    fprintf(stderr, "Avoided transfers: %7llu\n", avoidedTransfers);
#endif  /* DEBUG_TIMINGS */

    /* Free memory and types. */
    vllpaFinishTypes();
    vllpaFinishMemory();
    smallHashTableFinish();

    /* Debugging output. */
    printMemoryDataDependenceStats();
    PDEBUGLITE("Finished VLLPA\n");
}
