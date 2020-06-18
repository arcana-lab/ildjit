/*
 * Copyright (C) 2010  Campanoni Simone
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
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_nativemethods.h>
#include <config.h>
// End

#define INFORMATIONS    "This is a nativemethods plugin"
#define AUTHOR          "Campanoni Simone"

static inline JITUINT64 nativemethods_get_ID_job (void);
static inline char * nativemethods_get_version (void);
static inline void nativemethods_do_job (ir_method_t * method);
static inline char * nativemethods_get_informations (void);
static inline char * nativemethods_get_author (void);
static inline JITUINT64 nativemethods_get_dependences (void);
static inline void nativemethods_shutdown (JITFLOAT32 totalTime);
static inline void nativemethods_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 nativemethods_get_invalidations (void);
static inline void nativemethods_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void nativemethods_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline void nativemethods_inline_native_clib_methods (ir_method_t * method);
static inline void internal_inline_call_parameters (ir_method_t *method, ir_instruction_t *inst, JITUINT32 instType, JITUINT32 parsNum);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;

ir_optimization_interface_t plugin_interface = {
    nativemethods_get_ID_job,
    nativemethods_get_dependences,
    nativemethods_init,
    nativemethods_shutdown,
    nativemethods_do_job,
    nativemethods_get_version,
    nativemethods_get_informations,
    nativemethods_get_author,
    nativemethods_get_invalidations,
    NULL,
    nativemethods_getCompilationFlags,
    nativemethods_getCompilationTime
};

static inline void nativemethods_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

static inline void nativemethods_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
}

static inline JITUINT64 nativemethods_get_ID_job (void) {
    return NATIVE_METHODS_ELIMINATION;
}

static inline JITUINT64 nativemethods_get_dependences (void) {
    return 0;
}

static inline void nativemethods_do_job (ir_method_t * method) {

    /* Inline native methods that belong to the C library.
     */
    nativemethods_inline_native_clib_methods(method);

    return ;
}

static inline void nativemethods_inline_native_clib_methods (ir_method_t * method) {
    JITUINT32 instructionsNumber;
    JITUINT32 count;

    /* Assertions.
     */
    assert(method != NULL);

    /* Fetch the instructions number.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Inline native methods.
     */
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        ir_method_t             *calledMethod;
        ir_item_t		*fpNotResolved;
        ir_item_t		fpResolved;
        JITINT8                 *methodName;
        JITBOOLEAN		validCall;
        void			*fp;

        /* Fetch the instruction.
         */
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check the type of the instruction.
         */
        validCall	= JITFALSE;
        switch (inst->type) {
            case IRCALL:
            case IRLIBRARYCALL:
            case IRNATIVECALL:
                validCall	= JITTRUE;
                break;
        }
        if (!validCall) {
            continue ;
        }

        /* Fetch the callee.
         */
        methodName	= NULL;
        fp		= NULL;
        switch (inst->type) {
            case IRCALL:
            case IRLIBRARYCALL:
                calledMethod	= IRMETHOD_getCalledMethod(method, inst);
                if (	(calledMethod == NULL)					||
                        (!IRPROGRAM_doesMethodBelongToALibrary(calledMethod))	) {
                    continue;
                }
                assert(IRPROGRAM_doesMethodBelongToALibrary(calledMethod));
                methodName = IRMETHOD_getSignatureInString(calledMethod);
                break ;
            case IRNATIVECALL:
                fpNotResolved	= IRMETHOD_getInstructionParameter3(inst);
                if (IRDATA_isASymbol(fpNotResolved)) {
                    IRSYMBOL_resolveSymbolFromIRItem(fpNotResolved, &fpResolved);
                } else {
                    memcpy(&fpResolved, fpNotResolved, sizeof(ir_item_t));
                }
                fp	= (void *)(JITNUINT)fpResolved.value.v;
                break ;
        }
        if (	(methodName == NULL)	&&
                (fp == NULL)		) {
            continue;
        }

        /* Check the callee.
         */
        if (		((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.IntPtr System.Runtime.InteropServices.Marshal.AllocHGlobal(System.IntPtr cb)") == 0)	||
                         (STRCMP(methodName, "System.Void* libstd.malloc(System.UInt32 size)") == 0)						||
                         (STRCMP(methodName, "System.Void* MSCorelibWrapper.malloc(System.UInt32 size)") == 0)
                     )
              )		||
                    (
                        (fp == malloc)
                    )													) {
            internal_inline_call_parameters(method, inst, IRALLOC, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Void System.Runtime.InteropServices.Marshal.FreeHGlobal(System.IntPtr hglobal)") == 0)	||
                         (STRCMP(methodName, "System.Void libstd.free(System.Void* ptr)") == 0)							||
                         (STRCMP(methodName, "System.Void MSCorelibWrapper.free(System.Void* ptr)") == 0)
                     )
                    )		||
                    (
                        (fp == free)
                    )													) {
            internal_inline_call_parameters(method, inst, IRFREE, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.IntPtr System.Runtime.InteropServices.Marshal.ReAllocHGlobal(System.IntPtr pv,System.IntPtr cb)") == 0)	||
                         (STRCMP(methodName, "System.Void* libstd.realloc(System.Void* ptr,System.UInt32 size)") == 0)
                     )
                    )		||
                    (
                        (fp == realloc)
                    )													) {
            internal_inline_call_parameters(method, inst, IRREALLOC, 2);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Sqrt(System.Double a)") == 0)		||
                         (STRCMP(methodName, "System.Double libstd.sqrt(System.Double p0)") == 0)		||
                         (STRCMP(methodName, "System.Double MSCorelibWrapper.sqrt(System.Double d)") == 0)
                     )
                    )		||
                    (
                        (fp == sqrt)
                    )													) {
            internal_inline_call_parameters(method, inst, IRSQRT, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Sin(System.Double a)") == 0)		||
                         (STRCMP(methodName, "System.Double libstd.sin(System.Double p0)") == 0)			||
                         (STRCMP(methodName, "System.Double MSCorelibWrapper.sin(System.Double d)") == 0)
                     )
                    )		||
                    (
                        (fp == sin)
                    )													) {
            internal_inline_call_parameters(method, inst, IRSIN, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Cos(System.Double d)") == 0)		||
                         (STRCMP(methodName, "System.Double libstd.cos(System.Double p0)") == 0)			||
                         (STRCMP(methodName, "System.Double MSCorelibWrapper.cos(System.Double d)") == 0)
                     )
                    )		||
                    (
                        (fp == cos)
                    )													) {
            internal_inline_call_parameters(method, inst, IRCOS, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Acos(System.Double d)") == 0)		||
                         (STRCMP(methodName, "System.Double libstd.acos(System.Double p0)") == 0)		||
                         (STRCMP(methodName, "System.Double MSCorelibWrapper.acos(System.Double d)") == 0)
                     )
                    )		||
                    (
                        (fp == acos)
                    )													) {
            internal_inline_call_parameters(method, inst, IRACOS, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Int32 System.Double.TestInfinity(System.Double d)") == 0)	||
                         (STRCMP(methodName, "System.Boolean System.Double.IsInfinity(System.Double d)") == 0)
                     )
                    )		||
                    (
                        (fp == isinf)
                    )													) {
            internal_inline_call_parameters(method, inst, IRISINF, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Pow(System.Double x,System.Double y)") == 0)				||
                         (STRCMP(methodName, "System.Double libstd.pow(System.Double p0,System.Double p1)") == 0)
                     )
                    )		||
                    (
                        (fp == pow)
                    )													) {
            internal_inline_call_parameters(method, inst, IRPOW, 2);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Exp(System.Double d)") == 0)			||
                         (STRCMP(methodName, "System.Double libstd.exp(System.Double p0)") == 0)
                     )
                    )		||
                    (
                        (fp == exp)
                    )													) {
            internal_inline_call_parameters(method, inst, IREXP, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Log10(System.Double d)") == 0)			||
                         (STRCMP(methodName, "System.Double libstd.log10(System.Double p0)") == 0)
                     )
                    )		||
                    (
                        (fp == log10)
                    )													) {
            internal_inline_call_parameters(method, inst, IRLOG10, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Ceiling(System.Double a)") == 0)			||
                         (STRCMP(methodName, "System.Double libstd.ceil(System.Double p0)") == 0)
                     )
                    )		||
                    (
                        (fp == ceil)
                    )													) {
            internal_inline_call_parameters(method, inst, IRCEIL, 1);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.Double System.Math.Cosh(System.Double value)") == 0))	||
                    (fp == cosh)													) {
            internal_inline_call_parameters(method, inst, IRCOSH, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Double System.Math.Floor(System.Double d)") == 0)			||
                         (STRCMP(methodName, "System.Double libstd.floor(System.Double p0)") == 0)
                     )
                    )		||
                    (
                        (fp == floor)
                    )													) {
            internal_inline_call_parameters(method, inst, IRFLOOR, 1);

        } else if (	((methodName != NULL) &&
                     (
                         (STRCMP(methodName, "System.Void Platform.TaskMethods.Exit(System.Int32 exitCode)") == 0)
                     )
                    )		||
                    (
                        (fp == exit)
                    )													) {
            internal_inline_call_parameters(method, inst, IREXIT, 1);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.Boolean System.Double.IsNaN(System.Double d)") == 0))	||
                    (fp == isnan)													) {
            internal_inline_call_parameters(method, inst, IRISNAN, 1);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.Int32 System.Array.GetLength()") == 0))							) {
            internal_inline_call_parameters(method, inst, IRARRAYLENGTH, 1);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.UInt32 libstd.strlen(System.SByte* s)") == 0))) {
            internal_inline_call_parameters(method, inst, IRSTRINGLENGTH, 1);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.Int32 libstd.strcmp(System.SByte* s1,System.SByte* s2)") == 0))) {
            internal_inline_call_parameters(method, inst, IRSTRINGCMP, 2);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.SByte* libstd.strchr(System.SByte* s,System.Int32 c)") == 0))) {
            internal_inline_call_parameters(method, inst, IRSTRINGCHR, 2);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.Int32 libstd.memcmp(System.Void* s1,System.Void* s2,System.UInt32 n)") == 0))) {
            internal_inline_call_parameters(method, inst, IRMEMCMP, 3);

        } else if (	((methodName != NULL) && (STRCMP(methodName, "System.Void* libstd.calloc(System.UInt32 nmemb,System.UInt32 size)") == 0))) {
            internal_inline_call_parameters(method, inst, IRCALLOC, 2);

        } else {
            PDEBUG("NATIVE METHODS: Method not inlined = %s\n", (char *) methodName);
        }
    }

    return;
}

static inline void internal_inline_call_parameters (ir_method_t *method, ir_instruction_t *inst, JITUINT32 instType, JITUINT32 parsNum) {
    ir_item_t 	output;
    ir_item_t	*params;
    JITUINT32 	count;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);

    /* Fetch the output.
     */
    memcpy(&output, &(inst->result), sizeof(ir_item_t));

    /* Fetch the input.
     */
    params	= NULL;
    if (parsNum > 0) {

        /* Allocate the memory.
         */
        params	= allocFunction(sizeof(ir_item_t) * parsNum);

        /* Copy the parameters.
         */
        for (count=0; count < parsNum; count++) {
            ir_item_t	*callParameter;
            callParameter = IRMETHOD_getInstructionCallParameter(inst, count);
            assert(callParameter != NULL);
            memcpy(&(params[count]), callParameter, sizeof(ir_item_t));
        }
    }

    /* Erase the call instruction.
     */
    IRMETHOD_eraseInstructionFields(method, inst);

    /* Copy the input.
     */
    for (count=1; count <= parsNum; count++) {
        ir_item_t	*par;
        assert(params != NULL);
        par	= IRMETHOD_getInstructionParameter(inst, count);
        memcpy(par, &(params[count - 1]), sizeof(ir_item_t));
    }

    /* Copy the output.
     */
    IRMETHOD_cpInstructionResult(inst, &output);

    /* Set the instruction type.
     */
    inst->type = instType;

    /* Free the memory.
     */
    freeFunction(params);

    return;
}

static inline char * nativemethods_get_version (void) {
    return VERSION;
}

static inline char * nativemethods_get_informations (void) {
    return INFORMATIONS;
}

static inline char * nativemethods_get_author (void) {
    return AUTHOR;
}

static inline JITUINT64 nativemethods_get_invalidations (void) {
    return INVALIDATE_ALL & ~(ESCAPES_ANALYZER);
}

static inline void nativemethods_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

static inline void nativemethods_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
