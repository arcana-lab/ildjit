/*
 * Copyright (C) 2008 - 2011  Campanoni Simone
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
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <libgen.h>
#include <string.h>
#include <compiler_memory_manager.h>
#include <plugin_manager.h>
#include <platform_API.h>

// My headers
#include <config.h>
#include <ir_optimizer.h>
#include <ir_optimization_interface.h>
#include <optimization_levels.h>
#include <optimizations_utilities.h>
// End

#define PATH_OPTIMIZATIONS_PLUGIN get_path_optimizations_plugin()
#define PATH_OPTIMIZATION_LEVELS_PLUGINS get_path_optimization_levels_plugin()
#define PLUGIN_SYMBOL "plugin_interface"
#define DIM_BUF 1024

static inline void init_plugins (ir_optimizer_t *lib, ir_lib_t *irLib, char *outputPrefix);
static inline void internal_load_optimizations (ir_optimizer_t *lib);
static inline void internal_load_optimization_levels (ir_optimizer_t *lib);
static inline void internal_unload_optimizations (ir_optimizer_t *lib, JITFLOAT32 totalTime);
static inline void internal_unload_optimization_levels (ir_optimizer_t *lib, JITFLOAT32 totalTime);
static inline char *jobToString (JITUINT64 job);
static inline void parse_input (optimizations_t *opts, JITINT8 defaultValue);
static inline void enable_optimization (optimizations_t *opts, JITINT8 *name, JITINT32 length);
static inline char * get_path_optimizations_plugin ();
static inline char * get_path_optimization_levels_plugin ();
static inline void merge_input (ir_optimizer_t *lib);
static inline void free_methods_memory ();
static inline void allocate_methods_memory ();
static inline JITUINT32 internal_optimizeMethodAtAOTLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint, JITBOOLEAN postAOT);
static inline JITUINT32 internal_codetoolIDHashFunction (void *codetoolID);
static inline JITINT32 internal_codetoolIDEqualsFunction(void *key1, void *key2);

char prefix[256] = "\0", path_optimizations_plugin[256] = "\0";
char path_optimization_levels_plugin[256] = "\0";
ir_optimizer_t	*irOpt = NULL;

void IROptimizerNew (ir_optimizer_t *lib, ir_lib_t *irLib, JITINT32 verbose, JITUINT32 optimizationLevel, char *outputPrefix) {

    /* Assertions				*/
    assert(lib != NULL);
    assert(irLib != NULL);
    assert(outputPrefix != NULL);

    /* Fill up the structure		*/
    lib->verbose = verbose;
    lib->optimizationLevel = optimizationLevel;
    lib->plugins = NULL;

    /* Load the optimizations		*/
    internal_load_optimizations(lib);

    /* Load the optimization levels		*/
    internal_load_optimization_levels(lib);

    /* Parse the input			*/
    parse_input(&(lib->enabledOptimizations), JITTRUE);
    parse_input(&(lib->disabledOptimizations), JITFALSE);
    merge_input(lib);

    /* Call the init function of the plugins*/
    init_plugins(lib, irLib, outputPrefix);
    irOpt	= lib;

    return;
}

void IROPTIMIZER_shutdown (ir_optimizer_t *lib, JITFLOAT32 totalTime) {

    /* Assertions						*/
    assert(lib != NULL);

    /* Free the plugins					*/
    internal_unload_optimizations(lib, totalTime);
    internal_unload_optimization_levels(lib, totalTime);
    if (lib->jobCodeToolsMap != NULL) {
        xanHashTable_destroyTableAndKey(lib->jobCodeToolsMap);
        lib->jobCodeToolsMap	= NULL;
    }

    return;
}

void IROPTIMIZER_optimizeMethodAtAOTLevel (ir_optimizer_t *lib, ir_method_t *startMethod) {
    XanVar *checkPoint;

    /* Optimize the method					*/
    checkPoint = xanVar_new(allocFunction, freeFunction);
    xanVar_write(checkPoint, JITFALSE);
    IROPTIMIZER_optimizeMethodAtAOTLevel_checkpointable(lib, startMethod, JOB_START, checkPoint);

    /* Free the memory					*/
    xanVar_destroyVar(checkPoint);

    /* Return						*/
    return;
}

void IROPTIMIZER_optimizeMethodAtPostAOTLevel (ir_optimizer_t *lib, ir_method_t *method) {
    XanVar *checkPoint;

    /* Optimize the method					*/
    checkPoint = xanVar_new(allocFunction, freeFunction);
    xanVar_write(checkPoint, JITFALSE);
    IROPTIMIZER_optimizeMethodAtPostAOTLevel_checkpointable(lib, method, JOB_START, checkPoint);

    /* Free the memory					*/
    xanVar_destroyVar(checkPoint);

    /* Return						*/
    return;
}

void IROPTIMIZER_optimizeMethodAtLevel (ir_optimizer_t *lib, ir_method_t *method, JITINT32 optimizationLevel) {
    XanVar *checkPoint;

    /* Optimize the method					*/
    checkPoint = xanVar_new(allocFunction, freeFunction);
    xanVar_write(checkPoint, JITFALSE);
    IROPTIMIZER_optimizeMethodAtLevel_checkpointable(lib, method, optimizationLevel, JOB_START, checkPoint);

    /* Free the memory					*/
    xanVar_destroyVar(checkPoint);

    /* Return						*/
    return;
}

JITUINT32 IROPTIMIZER_optimizeMethodAtAOTLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint) {
    return internal_optimizeMethodAtAOTLevel_checkpointable(lib, startMethod, state, checkPoint, JITFALSE);
}

JITUINT32 IROPTIMIZER_optimizeMethodAtPostAOTLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint) {
    return internal_optimizeMethodAtAOTLevel_checkpointable(lib, startMethod, state, checkPoint, JITTRUE);
}

static inline JITUINT32 internal_optimizeMethodAtAOTLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint, JITBOOLEAN postAOT) {
    JITBOOLEAN prev_modified;
    XanList *l;
    XanList *flags_list;
    XanListItem *item;
    XanListItem *item2;

    /* Assertions						*/
    assert(lib != NULL);
    assert(startMethod != NULL);

    /* Cache the previous flags				*/
    prev_modified = startMethod->modified;
    flags_list = xanList_new(allocFunction, freeFunction, NULL);
    l = IRPROGRAM_getIRMethods();
    assert(l != NULL);
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t *method;
        method = item->data;
        assert(method != NULL);
        xanList_append(flags_list, (void *) (JITNUINT) method->modified);
        item = item->next;
    }

    /* Allocate the memory					*/
    allocate_methods_memory();

    /* Optimize the method					*/
    if (postAOT) {
        state = optimizeMethod_PostAOT_checkpointable(lib, startMethod, state, checkPoint);
    } else {
        state = optimizeMethod_AOT_checkpointable(lib, startMethod, state, checkPoint);
    }

    /* Reset the flags					*/
    if (prev_modified) {
        startMethod->modified = prev_modified;
    }
    item = xanList_first(l);
    item2 = xanList_first(flags_list);
    while (item != NULL) {
        ir_method_t *method;
        method = item->data;
        assert(method != NULL);
        method->modified = (JITBOOLEAN) (JITNUINT) item2->data;
        item = item->next;
        item2 = item2->next;
    }

    /* Free the memory					*/
    free_methods_memory();
    xanList_destroyList(l);
    xanList_destroyList(flags_list);

    /* Return                                               */
    return state;
}

JITUINT32 IROPTIMIZER_optimizeMethodAtLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint) {
    JITINT32 toUnlock;
    JITBOOLEAN prev_modified;

    /* Assertions						*/
    assert(lib != NULL);
    assert(method != NULL);

    /* Check if there is one plugin at least		*/
    if (    (lib->plugins == NULL)                          ||
            (xanList_length(lib->plugins) == 0)       ) {
        return JOB_END;
    }

    /* Cache the previous flags				*/
    prev_modified = method->modified;

    /* Lock the IR method					*/
    toUnlock = (IRMETHOD_tryLock(method) == 0);

    /* Allocate the memory					*/
    if (optimizationLevel > 0 && state == JOB_START) {
        IRMETHOD_allocateMethodExtraMemory(method);
    }

    /* Optimize the method					*/
    method->valid_optimization = 0;
    if (lib->verbose) {
        printf("LIBILJITIROPTIMIZER: Optimize the method %s\n", IRMETHOD_getMethodName(method));
    }
    state = optimizeMethod_atLevel(lib, method, optimizationLevel, state, checkPoint);

    /* Print the CFG					*/
    if (lib->verbose && state == JOB_END) {
        internal_callMethodOptimization(lib, method, METHOD_PRINTER, JITTRUE);
    }

    /* Free extra data used to optimize the method		*/
    if (optimizationLevel > 0 && state == JOB_END) {
        IRMETHOD_destroyMethodExtraMemory(method);
    }

    if (lib->verbose && state == JOB_END) {
        printf("LIBILJITIROPTIMIZER:    Final information on the method %s\n", IRMETHOD_getSignatureInString(method));
        printf("LIBILJITIROPTIMIZER:            Instructions    = %d\n", IRMETHOD_getInstructionsNumber(method));
        printf("LIBILJITIROPTIMIZER:            Variables       = %d\n", IRMETHOD_getNumberOfVariablesNeededByMethod(method));
    }

    /* Reset the flag					*/
    if (prev_modified) {
        method->modified = prev_modified;
    }

    /* Unlock the IR method					*/
    if (toUnlock) {
        IRMETHOD_unlock(method);
    }
    if (lib->verbose && state == JOB_END) {
        printf("LIBILJITIROPTIMIZER:  The method has been optimized\n");
    }

    /* Return						*/
    return state;
}

static inline void init_plugins (ir_optimizer_t *lib, ir_lib_t *irLib, char *outputPrefix) {
    XanListItem     *item;
    JITBOOLEAN	changed;
    JITBOOLEAN	*firstOpt;

    /* Assertions						*/
    assert(irLib != NULL);
    assert(lib != NULL);
    assert(outputPrefix != NULL);

    /* Cache some pointers.
     */
    firstOpt		= &((lib->enabledOptimizations).ddg);

    /* Create the maps.
     */
    lib->jobCodeToolsMap	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, internal_codetoolIDHashFunction, internal_codetoolIDEqualsFunction);

    /* Map all plugins to their jobs.
     * Enable all dependences of plugins that perform all enabled jobs.
     * Call all the codetools.
     */
    do {
        changed	= JITFALSE;
        item 	= xanList_first(lib->plugins);
        while (item != NULL) {
            ir_optimization_t 	*plugin;
            JITUINT64		jobKind;
            JITUINT64		tmp;
            JITUINT32		count;
            JITBOOLEAN		added;

            /* Fetch a plugin.
             */
            plugin	= (ir_optimization_t *) item->data;
            assert(plugin != NULL);
            assert(plugin->plugin != NULL);
            jobKind	= plugin->plugin->get_job_kind();

            /* Map all current unmapped-jobs that the current codetool is able to provide.
             */
            added	= JITFALSE;
            count	= 0;
            tmp	= 0x1;
            while (count < 64) {

                /* Check if the current plugin can provide this current job.
                 * Check if the current job is not assigned to anyone yet.
                 * Check if the current job has been enabled.
                 */
                if (	((jobKind & tmp) != 0)						&&
                        (internal_isOptimizationEnabled(lib, tmp, JITFALSE))		&&
                        (xanHashTable_lookup(lib->jobCodeToolsMap, &tmp) == NULL)	) {
                    JITUINT64	*key;
                    JITUINT64	deps;
                    JITUINT64	tmp2;
                    JITUINT32	count2;

                    /* The current codetool provides a job that is currently unmapped.
                     * Assign this job to it.
                     */
                    key		= allocFunction(sizeof(JITUINT64));
                    (*key)		= tmp;
                    plugin->enabled	= JITTRUE;
                    added		= JITTRUE;
                    xanHashTable_insert(lib->jobCodeToolsMap, key, plugin);

                    /* Enable all dependences of the current plugin.
                     */
                    deps	= plugin->plugin->get_dependences();
                    tmp2	= 0x1;
                    count2	= 0;
                    while (count2 < 64) {
                        if ((deps & tmp2) != 0) {

                            /* tmp2 is a dependence of the current plugin.
                             * Enable it.
                             */
                            (*(firstOpt + count2))	= JITTRUE;
                        }
                        count2++;
                        tmp2	= tmp2 << 1;
                    }
                }

                /* Fetch the next element.
                 */
                count++;
                tmp	= tmp << 1;
            }
            changed	|= added;

            /* Check whether some codetool job has been assigned to the plugin.
             */
            if (added) {

                /* Call the plugin.
                 */
                plugin->plugin->init(irLib, lib, outputPrefix);
            }

            /* Fetch the next plugin.
             */
            item = item->next;
        }

    } while (changed);

    /* Initialize optimization level plugins.
     */
    initialize_optimizationLevels_plugins(lib);

    return ;
}

static inline void shutdown_optimization_levels_plugin (ildjit_plugin_t *plugin, void *data) {
    JITFLOAT32 totalTime;
    ir_optimization_levels_t *opt_plugin;

    /* Assertions			*/
    assert(plugin != NULL);
    assert(data != NULL);

    /* Fetch the plugin		*/
    opt_plugin = (ir_optimization_levels_t *) plugin;
    assert(opt_plugin != NULL);

    /* Fetch the total time		*/
    totalTime = *((JITFLOAT32 *) data);

    /* Call the shutdown function	*/
    opt_plugin->plugin->shutdown(totalTime);

    /* Return			*/
    return;
}

static inline void shutdown_optimization_plugin (ildjit_plugin_t *plugin, void *data) {
    JITFLOAT32 totalTime;
    ir_optimization_t *opt_plugin;

    /* Assertions			*/
    assert(plugin != NULL);
    assert(data != NULL);

    /* Fetch the plugin		*/
    opt_plugin = (ir_optimization_t *) plugin;
    assert(opt_plugin != NULL);

    /* Check if the current plugin has been enabled.
     */
    if (opt_plugin->enabled) {

        /* Fetch the total time		*/
        totalTime = *((JITFLOAT32 *) data);

        /* Call the shutdown function	*/
        opt_plugin->plugin->shutdown(totalTime);
    }

    return;
}

static inline void internal_unload_optimization_levels (ir_optimizer_t *lib, JITFLOAT32 totalTime) {

    /* Assertions.
     */
    assert(lib != NULL);

    /* Unload the plugins	*/
    if (lib->optimizationLevelsPlugins != NULL) {
        PLUGIN_unloadPluginsFromDirectory(lib->optimizationLevelsPlugins, shutdown_optimization_levels_plugin, (void *) &totalTime);
        lib->optimizationLevelsPlugins = NULL;
    }

    return;
}

static inline void internal_unload_optimizations (ir_optimizer_t *lib, JITFLOAT32 totalTime) {

    /* Assertions.
     */
    assert(lib != NULL);

    /* Unload the plugins	*/
    if (lib->plugins != NULL) {
        PLUGIN_unloadPluginsFromDirectory(lib->plugins, shutdown_optimization_plugin, (void *) &totalTime);
        lib->plugins = NULL;
    }

    return;
}

static inline void internal_load_optimizations (ir_optimizer_t *lib) {

    /* Assertions			*/
    assert(lib != NULL);

    /* Make the list of the plugins	*/
    lib->plugins = xanList_new(allocFunction, freeFunction, NULL);
    assert(lib->plugins != NULL);

    /* Load the plugins		*/
    PLUGIN_loadPluginsFromDirectories(lib->plugins, (JITINT8 *) PATH_OPTIMIZATIONS_PLUGIN, sizeof(ir_optimization_t), (JITINT8 *) PLUGIN_SYMBOL);

    /* Return			*/
    return;
}

static inline void internal_load_optimization_levels (ir_optimizer_t *lib) {

    /* Assertions			*/
    assert(lib != NULL);

    /* Make the list of the plugins	*/
    lib->optimizationLevelsPlugins = xanList_new(allocFunction, freeFunction, NULL);
    assert(lib->optimizationLevelsPlugins != NULL);

    /* Load the plugins		*/
    PLUGIN_loadPluginsFromDirectories(lib->optimizationLevelsPlugins, (JITINT8 *) PATH_OPTIMIZATION_LEVELS_PLUGINS, sizeof(ir_optimization_levels_t), (JITINT8 *) PLUGIN_SYMBOL);
    if (xanList_length(lib->optimizationLevelsPlugins) == 0) {
        fprintf(stderr, "ERROR: No plugin to define optimization levels are available inside %s\n", PATH_OPTIMIZATION_LEVELS_PLUGINS);
        abort();
    }

    /* Return			*/
    return;
}

static inline char *jobToString (JITUINT64 job) {
    switch (job) {
        case DDG_COMPUTER:
            return "Data Dependences computation              ";
        case DDG_PROFILE:
            return "Data Dependences profile                  ";
        case LOOP_PROFILE:
            return "Loop profile                              ";
        case INDIRECT_CALL_ELIMINATION:
            return "Indirect calls elimination                ";
        case CDG_COMPUTER:
            return "Control Dependencies computation          ";
        case LIVENESS_ANALYZER:
            return "Liveness analyzer                         ";
        case REACHING_EXPRESSIONS_ANALYZER:
            return "Reaching expressions analyzer             ";
        case REACHING_DEFINITIONS_ANALYZER:
            return "Reaching definitions analyzer             ";
        case REACHING_INSTRUCTIONS_ANALYZER:
            return "Reaching instruction analyzer             ";
        case AVAILABLE_EXPRESSIONS_ANALYZER:
            return "Available expressions analyzer            ";
        case DEADCODE_ELIMINATOR:
            return "Deadcode eliminator                       ";
        case IR_CHECKER:
            return "IR code checker                           ";
        case METHOD_PRINTER:
            return "Method printer                            ";
        case SCALARIZATION:
            return "Scalarization                             ";
        case CALL_DISTANCE_COMPUTER:
            return "Call distance computer                    ";
        case COMMON_SUBEXPRESSIONS_ELIMINATOR:
            return "Common subexpressions eliminator          ";
        case CONSTANT_PROPAGATOR:
            return "Constant propagator                       ";
        case COPY_PROPAGATOR:
            return "Copy propagator                           ";
        case DUMPCODE:
            return "Dump code                                 ";
        case ESCAPES_ANALYZER:
            return "Escapes variables analyzer                ";
        case ESCAPES_ELIMINATION:
            return "Escapes elimination                       ";
        case INSTRUCTIONS_SCHEDULER:
            return "Instructions scheduler                    ";
        case BRANCH_PREDICTOR:
            return "Branch predictor                          ";
        case BASIC_BLOCK_IDENTIFIER:
            return "Basic block identifier                    ";
        case NULL_CHECK_REMOVER:
            return "Check null remover                        ";
        case PRE_DOMINATOR_COMPUTER:
            return "Pre dominator computer                    ";
        case POST_DOMINATOR_COMPUTER:
            return "Post dominator computer                   ";
        case LOOP_UNSWITCHING:
            return "Loop unswitching                          ";
        case CONSTANT_FOLDING:
            return "Constant folding                          ";
        case LOOP_IDENTIFICATION:
            return "Loop identification                       ";
        case LOOP_INVARIANT_CODE_HOISTING:
            return "Loop invariant code hoisting              ";
        case LOOP_INVARIANT_VARIABLES_ELIMINATION:
            return "Loop invariant variables elimination      ";
        case LOOP_UNROLLING:
            return "Loop unrolling                            ";
        case LOOP_PEELING:
            return "Loop peeling                              ";
        case STRENGTH_REDUCTION:
            return "Strength reduction                        ";
        case LOOP_INVARIANTS_IDENTIFICATION:
            return "Loop invariants identification            ";
        case LOOP_TO_WHILE:
            return "From do-while to while loop               ";
        case INDUCTION_VARIABLES_IDENTIFICATION:
            return "Induction variables identification        ";
        case THREADS_EXTRACTION:
            return "Threads extraction                        ";
        case BOUNDS_CHECK_REMOVAL:
            return "Redundant array bounds check removal      ";
        case ALGEBRAIC_SIMPLIFICATION:
            return "Algebraic simplification                  ";
        case METHOD_INLINER:
            return "Method inliner                            ";
        case CONVERSION_MERGING:
            return "Conversion merging                        ";
        case NATIVE_METHODS_ELIMINATION:
            return "Native methods                            ";
        case VARIABLES_RENAMING:
            return "Variables renaming                        ";
        case VARIABLES_LIVE_RANGE_SPLITTING:
            return "Variables live range splitting            ";
        case SSA_CONVERTER:
            return "normal form to SSA form code conversion   ";
        case SSA_BACK_CONVERTER:
            return "SSA form to normal form code conversion   ";
        case CUSTOM:
            return "Custom                                    ";
    }

    return NULL;
}

void libiljitiroptimizerCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

void libiljitiroptimizerCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libiljitiroptimizerVersion () {
    return VERSION;
}

static inline void parse_input (optimizations_t *opts, JITINT8 defaultValue) {
    JITINT8         *buf;
    JITINT32 count;
    JITINT32 start;
    JITINT8         *tempName;
    JITINT32 tempLength;

    /* Assertions				*/
    assert(opts != NULL);

    /* Check if we have inputs		*/
    if (opts->optimizationsList == NULL) {
        memset(opts, defaultValue, sizeof(optimizations_t));
        opts->optimizationsList = NULL;
        return;
    }

    /* Parse the input			*/
    buf = opts->optimizationsList;
    count = 0;
    start = 0;
    while (buf[count] != '\0') {
        if (    (buf[count] == ',')     &&
                (start < count)         ) {

            /* Build up the name		*/
            tempLength = count - start;
            tempName = allocMemory(tempLength + 1);
            snprintf((char *) tempName, tempLength + 1, "%s", buf + start);
            tempName[tempLength] = '\0';

            /* Enable the optimization	*/
            enable_optimization(opts, tempName, tempLength);

            /* Free the memory		*/
            freeMemory(tempName);

            /* Reset the start counter	*/
            start = count + 1;
        }
        count++;
    }

    /* Build up the last name		*/
    tempLength = count - start;
    tempName = allocMemory(tempLength + 1);
    snprintf((char *) tempName, tempLength + 1, "%s", buf + start);

    /* Enable the last optimization	*/
    enable_optimization(opts, tempName, tempLength);

    /* Free the memory		*/
    freeMemory(tempName);

    /* Return			*/
    return;
}

static inline void enable_optimization (optimizations_t *opts, JITINT8 *name, JITINT32 length) {
    JITBOOLEAN	found;
    JITBOOLEAN	*enabled;
    JITUINT64	temp;

    /* Assertions			*/
    assert(opts != NULL);
    assert(name != NULL);

    /* Check the name		*/
    found	= JITFALSE;
    temp	= 0x1;
    enabled	= &(opts->ddg);
    while (JITTRUE) {
        JITINT8 *currentName;
        currentName	= (JITINT8 *) IROPTIMIZER_jobToShortName(temp);
        if (currentName == NULL) {
            break;
        }
        if (STRNCMP(name, currentName, length) == 0) {
            (*enabled)	= JITTRUE;
            found		= JITTRUE;
            break;
        }
        temp	= temp << 1;
        enabled++;
    }
    if (!found) {
        char buf2[DIM_BUF];
        snprintf(buf2, DIM_BUF, "Error: Optimization algorithm \"%s\" does not exist.\nPlease run iljit --help to see which optimizations are currently available.", name);
        print_err(buf2, 0);
        exit(1);
    }

    /* Return			*/
    return;
}

static inline void merge_input (ir_optimizer_t *lib) {
    JITBOOLEAN	*enabled;
    JITBOOLEAN	*disabled;
    void		*endPointer;

    /* Assertions			*/
    assert(lib != NULL);

    /* Fetch the starting pointers	*/
    disabled		= &((lib->disabledOptimizations).ddg);
    enabled			= &((lib->enabledOptimizations).ddg);
    endPointer		= &((lib->disabledOptimizations).optimizationsList);

    /* Merge the inputs		*/
    while (disabled != endPointer) {
        if ((*disabled)) {
            (*enabled)	= JITFALSE;
        }
        disabled++;
        enabled++;
    }

    return;
}

static inline char * get_path_optimizations_plugin () {

    /* Check if this is the first	*
     * time this function is called	*/
    if (path_optimizations_plugin[0] == '\0' ) {
        char    *additionalPath;

        /* This is the first time this  *
         * function is called		*/
#ifdef WIN32    /* Windows */
        strcpy(prefix, WIN_DEFAULT_PREFIX);
        /* Check if exist the		*
         * environment variable		*/
        additionalPath = getenv("ILDJIT_PLUGINS");

        /* NOTE: Windows Paths problem to be fixed */
        //relative path: ok for win, ko for msys
        // if (additionalPath != NULL) {
        //    sprintf(path_optimizations_plugin, "optimizers/ENV_VAR_SEPARATOR%s", additionalPath);
        //	} else{
        //        sprintf(path_optimizations_plugin, "optimizers/");
        // }

        //linux-like paths: ok for msys, ko for win
        //if (additionalPath != NULL) {
        //   sprintf(path_optimizations_plugin, "/local/bin/optimizers/ENV_VAR_SEPARATOR%s", additionalPath);
        //	} else{
        //        sprintf(path_optimizations_plugin, "/local/bin/optimizers/");
        // }


        if (additionalPath != NULL) {
            sprintf(path_optimizations_plugin, "%s/lib/iljit/optimizers/%c%s", prefix, ENV_VAR_SEPARATOR, additionalPath);
        } else {
            sprintf(path_optimizations_plugin, "%s/lib/iljit/optimizers/", prefix);
        }

#else    /* linux */
        if (prefix[0] == '\0' ) {
            if (PLATFORM_readlink("/proc/self/exe", prefix, sizeof(prefix)) == -1) {
                abort();
            }
            PLATFORM_dirname(PLATFORM_dirname(prefix));
        }

        /* Check if exist the		*
         * environment variable		*/
        additionalPath = getenv("ILDJIT_PLUGINS");
        if (additionalPath != NULL) {
            sprintf(path_optimizations_plugin, "%s/lib/iljit/optimizers/%c%s", prefix, ENV_VAR_SEPARATOR, additionalPath);
        } else {
            sprintf(path_optimizations_plugin, "%s/lib/iljit/optimizers/", prefix);
        }
#endif
    }
    return path_optimizations_plugin;
}

static inline char * get_path_optimization_levels_plugin () {

    /* Check if this is the first	*
     * time this function is called	*/
    if (path_optimization_levels_plugin[0] == '\0' ) {
        char    *additionalPath;

        /* Fetch the path		*/
        additionalPath = getenv("ILDJIT_OPTIMIZATION_LEVELS_PLUGINS");

        /* This is the first time this  *
         * function is called		*/
#ifdef WIN32    /* Windows */
        strcpy(prefix, WIN_DEFAULT_PREFIX);

#else    /* linux */
        if (prefix[0] == '\0' ) {
            if (PLATFORM_readlink("/proc/self/exe", prefix, sizeof(prefix)) == -1) {
                abort();
            }
            PLATFORM_dirname(PLATFORM_dirname(prefix));
        }
#endif

        /* Check if exist the		*
         * environment variable		*/
        if (additionalPath != NULL) {
            sprintf(path_optimization_levels_plugin, "%s/lib/iljit/optimization_levels/%c%s", prefix, ENV_VAR_SEPARATOR, additionalPath);
        } else {
            sprintf(path_optimization_levels_plugin, "%s/lib/iljit/optimization_levels/", prefix);
        }
    }

    /* Return			*/
    return path_optimization_levels_plugin;
}

void IROPTIMIZER_callMethodOptimization (ir_optimizer_t *lib, ir_method_t *method, JITUINT64 optimizationKind) {
    internal_callMethodOptimization(irOpt, method, optimizationKind, JITTRUE);
}

void IROPTIMIZER_optimizeMethod (ir_optimizer_t *lib, ir_method_t *method) {

    /* Assertions						*/
    assert(lib != NULL);
    assert(method != NULL);
    assert(lib->plugins != NULL);

    /* Optimize the method					*/
    IROPTIMIZER_optimizeMethodAtLevel(lib, method, lib->optimizationLevel);

    /* Return						*/
    return;
}

JITUINT32 IROPTIMIZER_optimizeMethod_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions						*/
    assert(lib != NULL);
    assert(method != NULL);
    assert(lib->plugins != NULL);

    /* Optimize the method					*/
    state = IROPTIMIZER_optimizeMethodAtLevel_checkpointable(lib, method, lib->optimizationLevel, state, checkPoint);

    /* Return						*/
    return state;
}

static inline void allocate_methods_memory () {
    XanList         *l;
    XanListItem     *item;

    /* Fetch the list of all IR	*
     * methods			*/
    l = IRPROGRAM_getIRMethods();
    assert(l != NULL);

    /* Search the main method	*/
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t     *method;

        /* Fetch the method		*/
        method = (ir_method_t *) item->data;
        assert(method != NULL);

        /* Align memory			*/
        IRMETHOD_allocateMethodExtraMemory(method);

        /* Fetch the next element of the*
         * list				*/
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(l);

    /* Return			*/
    return;
}

static inline void free_methods_memory () {
    XanList         *l;
    XanListItem     *item;

    /* Fetch the list of all IR	*
     * methods			*/
    l = IRPROGRAM_getIRMethods();
    assert(l != NULL);

    /* Search the main method	*/
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t     *method;

        /* Fetch the method		*/
        method = (ir_method_t *) item->data;
        assert(method != NULL);

        /* Align memory			*/
        IRMETHOD_destroyMethodExtraMemory(method);

        /* Fetch the next element of the*
         * list				*/
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(l);

    /* Return			*/
    return;
}

JITINT8 * IROPTIMIZER_getOptimizationleveltoolPaths (void) {
    return (JITINT8 *)PATH_OPTIMIZATION_LEVELS_PLUGINS;
}

JITINT8 * IROPTIMIZER_getCodetoolPaths (void) {
    return (JITINT8 *)PATH_OPTIMIZATIONS_PLUGIN;
}

char * IROPTIMIZER_jobToShortName (JITUINT64 job) {
    switch (job) {
        case DDG_COMPUTER:
            return "ddg";
        case DDG_PROFILE:
            return "ddgprof";
        case LOOP_PROFILE:
            return "loopprof";
        case CDG_COMPUTER:
            return "cdg";
        case INDIRECT_CALL_ELIMINATION:
            return "indirectcalls";
        case LIVENESS_ANALYZER:
            return "liveness";
        case REACHING_EXPRESSIONS_ANALYZER:
            return "reachexpr";
        case REACHING_INSTRUCTIONS_ANALYZER:
            return "reachinst";
        case REACHING_DEFINITIONS_ANALYZER:
            return "reachdefs";
        case AVAILABLE_EXPRESSIONS_ANALYZER:
            return "availableexpr";
        case DEADCODE_ELIMINATOR:
            return "deadce";
        case IR_CHECKER:
            return "codechecker";
        case METHOD_PRINTER:
            return "codeprinter";
        case SCALARIZATION:
            return "scal";
        case CALL_DISTANCE_COMPUTER:
            return "calldistance";
        case COMMON_SUBEXPRESSIONS_ELIMINATOR:
            return "cse";
        case CONSTANT_PROPAGATOR:
            return "consprop";
        case COPY_PROPAGATOR:
            return "copyprop";
        case DUMPCODE:
            return "codedump";
        case ESCAPES_ANALYZER:
            return "escapes";
        case ESCAPES_ELIMINATION:
            return "escapesdel";
        case INSTRUCTIONS_SCHEDULER:
            return "sched";
        case BRANCH_PREDICTOR:
            return "branchpredictor";
        case BASIC_BLOCK_IDENTIFIER:
            return "bb";
        case NULL_CHECK_REMOVER:
            return "nullcheck";
        case PRE_DOMINATOR_COMPUTER:
            return "predom";
        case POST_DOMINATOR_COMPUTER:
            return "postdom";
        case LOOP_IDENTIFICATION:
            return "loop";
        case LOOP_INVARIANT_CODE_HOISTING:
            return "loopinvarianthoisting";
        case LOOP_INVARIANT_VARIABLES_ELIMINATION:
            return "loopinvariantvars";
        case LOOP_UNROLLING:
            return "loopunroll";
        case LOOP_PEELING:
            return "looppeel";
        case STRENGTH_REDUCTION:
            return "strengthred";
        case LOOP_INVARIANTS_IDENTIFICATION:
            return "loopinvariant";
        case LOOP_UNSWITCHING:
            return "loopunswitch";
        case LOOP_TO_WHILE:
            return "loopwhile";
        case INDUCTION_VARIABLES_IDENTIFICATION:
            return "indvars";
        case THREADS_EXTRACTION:
            return "tlpextraction";
        case CONSTANT_FOLDING:
            return "cfold";
        case BOUNDS_CHECK_REMOVAL:
            return "arraybounds";
        case ALGEBRAIC_SIMPLIFICATION:
            return "algebraic";
        case METHOD_INLINER:
            return "methodinline";
        case CONVERSION_MERGING:
            return "convmerging";
        case NATIVE_METHODS_ELIMINATION:
            return "nativemethodinline";
        case VARIABLES_RENAMING:
            return "variablesrename";
        case VARIABLES_LIVE_RANGE_SPLITTING:
            return "varsrangesplit";
        case SSA_CONVERTER:
            return "tossa";
        case SSA_BACK_CONVERTER:
            return "fromssa";
        case CUSTOM:
            return "custom";
    }

    return NULL;
}

char * IROPTIMIZER_jobToString (JITUINT64 job) {
    return jobToString(job);
}

JITBOOLEAN IROPTIMIZER_isCodeToolInstalled (ir_optimizer_t *lib, JITUINT64 codetoolKind) {
    return internal_isOptimizationInstalled(lib, codetoolKind);
}

JITBOOLEAN IROPTIMIZER_canCodeToolBeUsed (ir_optimizer_t *lib, JITUINT64 codetoolKind) {
    if (!internal_isOptimizationEnabled(lib, codetoolKind, JITFALSE)) {
        return JITFALSE;
    }
    if (!internal_isOptimizationInstalled(lib, codetoolKind)) {
        return JITFALSE;
    }
    return JITTRUE;
}

void IROPTIMIZER_invalidateInformation (ir_method_t *method, JITUINT64 codetoolKind) {
    method->valid_optimization	&= ~(codetoolKind);

    return ;
}

void IROPTIMIZER_setInformationAsValid (ir_method_t *method, JITUINT64 codetoolKind){
    method->valid_optimization |= codetoolKind;

    return ;
}

JITBOOLEAN IROPTIMIZER_isInformationValid (ir_method_t *method, JITUINT64 codetoolKind) {
    return (method->valid_optimization & codetoolKind) == codetoolKind;
}

void IROPTIMIZER_startExecution (ir_optimizer_t *lib) {
    XanListItem     *item;

    /* Assertions						*/
    assert(lib != NULL);

    /* Call all the plugins					*/
    item = xanList_first(lib->plugins);
    while (item != NULL) {
        ir_optimization_t *plugin;

        /* Call a plugin				*/
        plugin = (ir_optimization_t *) item->data;
        assert(plugin != NULL);

        /* Call the plugin				*/
        if (plugin->plugin->startExecution != NULL) {
            plugin->plugin->startExecution();
        }

        /* Fetch the next plugin			*/
        item = item->next;
    }

    return ;
}

static inline JITUINT32 internal_codetoolIDHashFunction (void *codetoolID) {
    JITUINT64	*id;

    id	= codetoolID;
    return (JITUINT32) (*id);
}

static inline JITINT32 internal_codetoolIDEqualsFunction(void *key1, void *key2) {
    JITUINT64	*id1;
    JITUINT64	*id2;

    if (key1 == key2) {
        return JITTRUE;
    }
    if (	(key1 == NULL)	||
            (key2 == NULL)	) {
        return JITFALSE;
    }
    id1	= key1;
    id2	= key2;
    return ((*id1) == (*id2));
}
