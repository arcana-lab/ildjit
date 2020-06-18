/*
 * Copyright (C) 2012  Campanoni Simone
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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <ildjit.h>
#include <chiara.h>
#include <ir_method.h>
#include <cam.h>

// My headers
#include <optimizer_loop_trace.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a loop profile plugin"
#define	AUTHOR		"Simone Campanoni and Timothy Jones"
#define JOB            	LOOP_PROFILE


/**
 * Globals for this pass are all held in a single structure.
 **/
typedef struct {
    ir_lib_t *irLib;
    ir_optimizer_t *irOptimizer;
    char *prefix;
    XanHashTable *loops;              /**< All loops identified. */
    XanHashTable *loopNames;          /**< Names of all loops. */
    XanList *loopProfiles;            /**< Profile structures for all loops profiled. */
} pass_globals_t;


/**
 * The globals for this pass.
 **/
static pass_globals_t *globals = NULL;


/**
 * Convert from an integer to pointer and back.
 **/
#define uintToPtr(val) ((void *)(JITNUINT)(val))
#define ptrToUint(ptr) ((JITNUINT)(ptr))


/**
 * Start a loop (i.e. a new invocation).
 **/
static void
startLoop(loop_profile_t *profile) {
    CAM_profileLoopInvocationStart(ptrToUint(profile->userData));
}


/**
 * Dump the CFGs of all methods.
 **/
static void
dumpMethodCFGs(XanHashTable *methods)
{
    XanHashTableItem *methodItem = xanHashTable_first(methods);
    while (methodItem) {
        ir_method_t *method = methodItem->elementID;
        IROPTIMIZER_callMethodOptimization(globals->irOptimizer, method, METHOD_PRINTER);
        methodItem = xanHashTable_next(methods, methodItem);
    }
}


/**
 * Inject code into the relevant methods to profile instructions executed.
 **/
static void
injectCodeIntoMethods(XanList *loopsList, XanHashTable *profileMethods, XanHashTable *profileInsts) {
    XanList *paramList;
    XanListItem *loopItem;
    XanHashTable *seenMethods;
    ir_item_t param;

    /* Create the parameters for code injection. */
    paramList = xanList_new(allocFunction, freeFunction, NULL);
    memset(&param, 0, sizeof(ir_item_t));
    param.type = IRUINT32;
    param.internal_type = param.type;
    xanList_append(paramList, &param);

    /* To avoid profiling a method twice. */
    seenMethods = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Iterate over all starting methods. */
    loopItem = xanList_first(loopsList);
    while (loopItem) {
        loop_t *loop = loopItem->data;
        XanList *reachableMethods;
        XanListItem *methodItem;
        XanHashTable *methodsWithIRBody;

        /* Methods that have IR and so can be profiled. */
        methodsWithIRBody = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        reachableMethods = IRPROGRAM_getReachableMethods(loop->method);
        methodItem = xanList_first(reachableMethods);
        while (methodItem) {
            ir_method_t *method = methodItem->data;
            if (IRMETHOD_hasIRMethodBody(method)) {
                xanHashTable_insert(methodsWithIRBody, method, method);
            }
            methodItem = methodItem->next;
        }

        /* Consider the methods reachable from this. */
        methodItem = xanList_first(reachableMethods);
        while (methodItem) {
            ir_method_t *method = methodItem->data;
            if (!xanHashTable_lookup(seenMethods, method)) {
                JITINT8 *methodName = IRMETHOD_getSignatureInString(method);
                if (methodName != NULL &&
                    IRMETHOD_hasMethodInstructions(method) &&
                    STRCMP(IRMETHOD_getMethodName(method), ".cctor") != 0) {
                    XanList *instructions = IRMETHOD_getInstructions(method);
                    XanListItem *instItem = xanList_first(instructions);

                    /* Add code after required instructions. */
                    while (instItem) {
                        ir_instruction_t *inst = instItem->data;
                        XanHashTableItem *profileItem = xanHashTable_lookupItem(profileInsts, inst);
                        if (profileItem) {
							if (	IRMETHOD_isAMemoryInstruction(inst)	||
									IRMETHOD_isACallInstruction(inst)	){
								assert(inst->type != IRLABEL && inst->type != IRNOP && inst->type != IREXITNODE);

								/* Ignore some non-loop-method direct calls (needs updating if >1 loop method). */
								if (inst->type == IRCALL && method != loop->method) {
									XanList *calleeList = IRMETHOD_getCallableMethods(inst);
									XanListItem *calleeItem = xanList_first(calleeList);
									while (calleeItem) {
										IR_ITEM_VALUE calleeMethodId = (IR_ITEM_VALUE)(JITNUINT)calleeItem->data;
										ir_method_t *callee = IRMETHOD_getIRMethodFromMethodID(method, calleeMethodId);
										if (!xanHashTable_lookup(methodsWithIRBody, callee)) {
											param.value.v = ptrToUint(profileItem->element);
											IRMETHOD_newNativeCallInstructionBefore(method, inst, "seenInstruction", CAM_profileLoopSeenInstruction, NULL, paramList);
											break;
										}
										calleeItem = calleeItem->next;
									}
									xanList_destroyList(calleeList);
								}

								/* Profile all other instructions. */
								else {
									param.value.v = ptrToUint(profileItem->element);
									IRMETHOD_newNativeCallInstructionBefore(method, inst, "seenInstruction", CAM_profileLoopSeenInstruction, NULL, paramList);
								}
							}
                        }
                        instItem = instItem->next;
                    }

                    /* Clean up. */
                    xanList_destroyList(instructions);
                }
                xanHashTable_insert(seenMethods, method, method);
            }
            methodItem = methodItem->next;
        }

        /* Clean up. */
        xanHashTable_destroyTable(methodsWithIRBody);
        xanList_destroyList(reachableMethods);
        loopItem = loopItem->next;
    }

    /* Clean up. */
    xanList_destroyList(paramList);
    xanHashTable_destroyTable(seenMethods);
}


/**
 * Inject code into a method to profile call instructions.
 **/
static void
injectCallCodeIntoMethod(ir_method_t *method, XanHashTable *profileInsts)
{
  XanList *instructions;
  XanList *paramList;
  ir_item_t param;
  XanListItem *instItem;

  /* Create the parameters for code injection. */
  paramList = xanList_new(allocFunction, freeFunction, NULL);
  memset(&param, 0, sizeof(ir_item_t));
  param.type = IRUINT32;
  param.internal_type = param.type;
  xanList_append(paramList, &param);

  /* Iterate over method instructions. */
  instructions = IRMETHOD_getInstructions(method);
  instItem = xanList_first(instructions);
  while (instItem) {
    ir_instruction_t *inst = instItem->data;
    if (IRMETHOD_isACallInstruction(inst)) {
      XanHashTableItem *profileItem = xanHashTable_lookupItem(profileInsts, inst);
      if (profileItem){
         param.value.v = ptrToUint(profileItem->element);
         IRMETHOD_newNativeCallInstructionBefore(method, inst, "callInvocationStart", CAM_profileCallInvocationStart, NULL, paramList);
       	 IRMETHOD_newNativeCallInstructionAfter(method, inst, "callInvocationEnd", CAM_profileCallInvocationEnd, NULL, NULL);
      }
    }
    instItem = instItem->next;
  }

  /* Clean up. */
  xanList_destroyList(paramList);
  xanList_destroyList(instructions);
}


/**
 * Inject code into the program to profile loop invocations and iterations.
 **/
static void
injectCodeIntoProgram(void) {
    char *env;
    char *prevName;
    XanList *loopsList;
    XanListItem *profileItem;
    XanHashTable *loopDict;
    XanHashTable *instsToProfile;
    XanHashTable *methodsToConsider;
    JITINT32 dumpCFG = -1;

    /* Read the list of instructions to profile. */
    methodsToConsider = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    instsToProfile = MISC_loadInstructionDictionary(methodsToConsider, "instruction_IDs.txt", NULL, NULL);

    /* Determine whether to dump CFGs. */
    prevName = getenv("DOTPRINTER_FILENAME");
    env = getenv("LOOP_TRACE_DUMP_CFG");
    if (env) {
      dumpCFG = atoi(env);
    }

    /* Dump CFGs if necessary. */
    if (dumpCFG >= 0) {
      setenv("DOTPRINTER_FILENAME", "loop_trace", 1);
      dumpMethodCFGs(methodsToConsider);
    }

    /* Analyze the loops. */
    globals->loops = IRLOOP_analyzeLoops(LOOPS_analyzeCircuits);
    assert(globals->loops != NULL);
    globals->loopNames = LOOPS_getLoopsNames(globals->loops);
    assert(globals->loopNames != NULL);
    loopsList = xanHashTable_toList(globals->loops);
    assert(loopsList != NULL);

    /* Select the correct loop. */
    loopDict = MISC_loadLoopDictionary("loop_IDs.txt", NULL);
    MISC_chooseLoopsToProfile(loopsList, globals->loopNames, loopDict);
    if (xanList_length(loopsList) == 0) {
        fprintf(stderr, "LOOP TRACER: Error: No loops selected to profile\n");
        fprintf(stderr, "Please check that the file loop_IDs.txt exists in the working directory\n");
        fprintf(stderr, "and that environment variables PARALLELIZER_LOWEST_LOOPS_TO_PARALLELIZE\n");
        fprintf(stderr, "and PARALLELIZER_MAXIMUM_LOOPS_TO_PARALLELIZE have been set properly if\n");
        fprintf(stderr, "you are using an *_enable_loops file.\n");
        abort();
    } else if (xanList_length(loopsList) > 1) {
        fprintf(stderr, "LOOP TRACER: Warning: More than one loop selected to profile\n");
        fprintf(stderr, "This plugin may not work correctly with more than one loop\n");
    }

    /* Massage the code.    */
    if (xanList_length(loopsList) > 0){
        XanList     *methods;
        XanListItem *item;

        methods = LOOPS_getMethods(loopsList);
        assert(methods != NULL); 
        item = xanList_first(methods);
        while (item != NULL) {
            ir_method_t     *m;
            m = item->data;
            assert(m != NULL);
            METHODS_addEmptyBasicBlocksJustBeforeInstructionsWithMultiplePredecessors(m);
            IROPTIMIZER_callMethodOptimization(NULL, m, LOOP_IDENTIFICATION);
            item = item->next;
        }
        xanList_destroyList(methods);

        IRLOOP_refreshLoopsInformation(globals->loops);
    }

    /* Profile the loops. */
    globals->loopProfiles = LOOPS_profileLoopBoundaries(globals->irOptimizer, loopsList, JITFALSE, globals->loopNames, startLoop, CAM_profileLoopInvocationEnd, CAM_profileLoopIterationStart, NULL);

    /* Dump CFGs if necessary. */
    if (dumpCFG >= 3) {
      dumpMethodCFGs(methodsToConsider);
    }

    /* Profile all instructions in all methods, before call profiling. */
    injectCodeIntoMethods(loopsList, methodsToConsider, instsToProfile);

    /* Dump CFGs if necessary. */
    if (dumpCFG >= 2) {
      dumpMethodCFGs(methodsToConsider);
    }

    /* Set up user data in the profile and profile call instructions. */
    profileItem = xanList_first(globals->loopProfiles);
    while (profileItem) {
        loop_profile_t *profile = profileItem->data;
        JITINT8 *loopName = xanHashTable_lookup(globals->loopNames, profile->loop);
        profile->userData = uintToPtr(xanHashTable_lookup(loopDict, loopName));
        assert(profile->userData != NULL);
        injectCallCodeIntoMethod(profile->loop->method, instsToProfile);
        profileItem = profileItem->next;
    }

    /* Dump CFGs if necessary. */
    if (dumpCFG >= 1) {
      dumpMethodCFGs(methodsToConsider);
    }

    /* Restore environment variables. */
    env = getenv("LOOP_TRACE_DUMP_CFG");
    if (env) {
      if (prevName != NULL) {
        setenv("DOTPRINTER_FILENAME", prevName, 1);
      } else {
        unsetenv("DOTPRINTER_FILENAME");
      }
    }

    /* Free the memory. */
    xanList_destroyList(loopsList);
    xanHashTable_destroyTable(loopDict);
    xanHashTable_destroyTable(instsToProfile);
    xanHashTable_destroyTable(methodsToConsider);
}


static inline void
loop_trace_init(ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    /* Assertions. */
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    /* Set up globals for this pass. */
    globals = allocFunction(sizeof(pass_globals_t));
    globals->irLib = lib;
    globals->irOptimizer = optimizer;
    globals->prefix = outputPrefix;
    globals->loops = NULL;
    globals->loopNames = NULL;

    /* Initialise the library. */
    CAM_init(CAM_LOOP_PROFILE);
}

static inline void
loop_trace_shutdown(JITFLOAT32 totalTime) {
    /* Shut down the library. */
    CAM_shutdown(CAM_LOOP_PROFILE);

    /* Clean up. */
    if (globals->loops) {
        IRLOOP_destroyLoops(globals->loops);
    }
    if (globals->loopNames) {
        LOOPS_destroyLoopsNames(globals->loopNames);
    }
    freeFunction(globals);
    globals = NULL;
}

static inline JITUINT64 loop_trace_get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 loop_trace_get_dependences (void) {
    return LOOP_IDENTIFICATION;
}

static inline void loop_trace_do_job (ir_method_t * method) {
    compilation_scheme_t 	cs;

    /* Assertions.
     */
    assert(method != NULL);

    /* Fetch the compilation scheme currently in use.
     */
    cs	= ILDJIT_compilationSchemeInUse();

    /* Check whether we are statically compiling the program.
     */
    if (!ILDJIT_isAStaticCompilationScheme(cs)) {
        return ;
    }
    if (IRPROGRAM_getEntryPointMethod() != method) {
        return ;
    }

    /* We are compiling the code with a static compilation scheme.
     * Inject the code to profile the execution of the entire program.
     */
    injectCodeIntoProgram();
}

static inline char * loop_trace_get_version (void) {
    return VERSION;
}

static inline char * loop_trace_get_informations (void) {
    return INFORMATIONS;
}

static inline char * loop_trace_get_author (void) {
    return AUTHOR;
}

static inline JITUINT64 loop_trace_get_invalidations (void) {
    return INVALIDATE_ALL;
}

static inline void loop_trace_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void loop_trace_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}

ir_optimization_interface_t plugin_interface = {
    loop_trace_get_ID_job		,
    loop_trace_get_dependences	,
    loop_trace_init		,
    loop_trace_shutdown		,
    loop_trace_do_job		,
    loop_trace_get_version		,
    loop_trace_get_informations	,
    loop_trace_get_author		,
    loop_trace_get_invalidations	,
    NULL				,
    loop_trace_getCompilationFlags	,
    loop_trace_getCompilationTime
};
