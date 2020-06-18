/*
 * Copyright (C) 2006  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <xanlib.h>
#include <iljit-utils.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>

// My headers
#include <ildjit_profiler.h>
#include <general_tools.h>
// End

#define DIM_BUF         1024


ILClockID internal_getClockID (void);
static inline void internal_print_cpu_time_and_wall_time (JITFLOAT32 CPUTime, JITFLOAT32 totalCPUTime, JITFLOAT32 wallTime, JITFLOAT32 totalWallTime);

ILClockID clockID = -1;

void profilerStartTime (ProfileTime *startTime) {

    /* Assertions			*/
    assert(startTime != NULL);

    /* Initialize the variables	*/
    memset(startTime, 0, sizeof(ProfileTime));

    /* Take the start time		*/
    if (clockID == -1) {
        clockID = getBestClockID();
    }

#ifdef PROFILE_CLOCKGETTIME
    PLATFORM_clock_gettime(clockID, &(startTime->time));
#endif
    gettimeofday(&(startTime->wallTime), NULL);
}

JITFLOAT64 profilerGetSeconds (ProfileTime *time) {
    JITFLOAT64 t1;

    t1 = (time->wallTime).tv_sec + ((time->wallTime).tv_usec / 1000000.0);

    return t1;
}

JITFLOAT32 profilerGetTimeElapsed (ProfileTime *startTime, JITFLOAT32 *wallTime) {
    ProfileTime stopTime;
    JITFLOAT32 timeElapsed;

    /* Assertions				*/
    assert(startTime != NULL);

    /* Store the stop time			*/
#ifdef PROFILE_CLOCKGETTIME
    PLATFORM_clock_gettime(clockID, &(stopTime.time));
#endif
    if (wallTime != NULL) {
        gettimeofday(&(stopTime.wallTime), NULL);
    }

    /* Store profiling informations		*/
#ifdef PROFILE_CLOCKGETTIME
    timeElapsed = diff_time(startTime->time, stopTime.time);
#endif
    if (wallTime != NULL) {
        JITFLOAT64 t2;
        JITFLOAT64 t1;
        t1 = (startTime->wallTime).tv_sec + ((startTime->wallTime).tv_usec / 1000000.0);
        t2 = stopTime.wallTime.tv_sec + (stopTime.wallTime.tv_usec / 1000000.0);
        (*wallTime) = t2 - t1;
    }

    /* Return				*/
    return timeElapsed;
}

void printProfilerInformation (void) {
    JITFLOAT32 temp1;
    JITFLOAT32 totalWallTime;
    JITFLOAT32 temp2;
    JITFLOAT32 temp2Wall;
    JITINT32 count;
    JITINT32 internalMethods;
    XanList                 *classes;
    XanList                 *staticClasses;
    XanList                 *ilTypes;
    XanList *methods;
    XanListItem     *item;
    t_gc_informations gcInfo;
    t_system                *system;

    /* Fetch the system                     */
    system = getSystem(NULL);
    assert(system != NULL);
    assert((system->garbage_collectors).gc.gcInformations != NULL);

    /* Fetch the GC informations            */
    memset(&gcInfo, 0, sizeof(t_gc_informations));
    (system->garbage_collectors).gc.gcInformations(&gcInfo);

    /* Print the profiling informations */
    fprintf(stderr, "#########################################################################\n");
    fprintf(stderr, "			PROFILING					\n");
    fprintf(stderr, "#########################################################################\n");
    fprintf(stderr, "Profiling clockID = ");
#ifdef PROFILE_CLOCKGETTIME
    switch (getBestClockID()) {
        case CLOCK_REALTIME:
            fprintf(stderr, "CLOCK_REALTIME\n");
            break;
        case CLOCK_MONOTONIC:
            fprintf(stderr, "CLOCK_MONOTONIC\n");
            break;
        case CLOCK_PROCESS_CPUTIME_ID:
            fprintf(stderr, "CLOCK_PROCESS_CPUTIME_ID\n");
            break;
        case CLOCK_THREAD_CPUTIME_ID:
            fprintf(stderr, "CLOCK_THREAD_CPUTIME_ID\n");
            break;
        default:
            print_err("ILJIT: ERROR = Clock ID is not known. ", 0);
            abort();
    }
#else
    fprintf(stderr, "Not available\n");
#endif

    fprintf(stderr, "Garbage Collector used  = %s\n", ((system->garbage_collectors).gc.gc_plugin)->getName());
    temp1 = (system->profiler).total_time;
    totalWallTime = (system->profiler).wallTime.total_time;

    /* Fetch the methods translated	*/
    methods = (system->cliManager).methods.container;
    assert(methods != NULL);

    /* Count the number of internal *
     * methods considered		*/
    item = xanList_first(methods);
    internalMethods = 0;
    while (item != NULL) {
        Method method;
        method = (Method) item->data;
        assert(method != NULL);
        if (!method->isIrImplemented(method)) {
            internalMethods++;
        }
        item = item->next;
    }

    /* Methods translated		*/
    fprintf(stderr, "Methods translation\n");
    fprintf(stderr, "        Total methods considered			= %d\n", xanList_length(methods));
    fprintf(stderr, "        Total internal methods                          = %d\n", internalMethods);
    fprintf(stderr, "        Total methods translated			= %d\n", (system->pipeliner).methodsCompiled);
    fprintf(stderr, "        Methods translated using high priority		= %d\n", (system->pipeliner).methodsCompiledInHighPriority);
    fprintf(stderr, "        Methods moved to high priority			= %d\n", (system->pipeliner).methodsMovedToHighPriority);
    fprintf(stderr, "        Total reprioritization				= %d\n", (system->pipeliner).reprioritizedTicket);
    fprintf(stderr, "        Total checkpointing				= %d\n", (system->pipeliner).rescheduledTask);
    fprintf(stderr, "        Trampolines taken\n");
    fprintf(stderr, "        	Before starting the execution of the entry point = %u\n", (system->profiler).trampolinesTakenBeforeEntryPoint);
    fprintf(stderr, "        	After starting the execution of the entry point  = %u\n", (system->profiler).trampolinesTaken);
    fprintf(stderr, "        Static methods executed				= %d\n", xanHashTable_elementsInside((system->staticMemoryManager).cctorMethodsExecuted));
    if ((system->IRVM).behavior.profiler >= 2) {
        fprintf(stderr, "	Total number of CIL opcoded decoded		= %d\n", (system->program).cil_opcodes_decoded);
        fprintf(stderr, "	Average number of CIL opcodes per method	= %d\n", (system->program).cil_opcodes_decoded / (system->pipeliner).methodsCompiled);
    }
    fprintf(stderr, "\n");

    /* Types layouted		*/
    fprintf(stderr, "Types\n");
    ilTypes = xanList_new(allocFunction, freeFunction, NULL);
    assert(ilTypes != NULL);
    classes = (system->cliManager).layoutManager.classesLayout;
    assert(classes != NULL);
    staticClasses = (system->cliManager).layoutManager.staticTypeLayout;
    assert(staticClasses != NULL);
    item = xanList_first(classes);
    while (item != NULL) {
        ILLayout        *layout;
        XanListItem     *item2;
        JITINT32 found;
        layout = (ILLayout *) item->data;
        assert(layout != NULL);
        found = 0;
        item2 = xanList_first(ilTypes);
        while (item2 != NULL) {
            TypeDescriptor  *ilType;
            ilType = xanList_getData(item2);
            assert(ilType != NULL);
            if (layout->type == ilType) {
                found = 1;
                break;
            }
            item2 = item2->next;
        }
        if (!found) {
            xanList_insert(ilTypes, layout->type);
        }
        item = item->next;
    }
    item = xanList_first(staticClasses);
    while (item != NULL) {
        ILLayoutStatic  *layout;
        XanListItem     *item2;
        JITINT32 found;
        layout = (ILLayoutStatic *) item->data;
        assert(layout != NULL);
        found = 0;
        item2 = xanList_first(ilTypes);
        while (item2 != NULL) {
            TypeDescriptor  *ilType;
            ilType = xanList_getData(item2);
            assert(ilType != NULL);
            if (layout->type == ilType) {
                found = 1;
                break;
            }
            item2 = item2->next;
        }
        if (!found) {
            xanList_insert(ilTypes, layout->type);
        }
        item = item->next;
    }
    fprintf(stderr, "	Types loaded				= %d\n", xanList_length(ilTypes));
    if ((system->IRVM).behavior.profiler >= 2) {
        count = 0;
        XanListItem *list_item = xanList_first(ilTypes);
        while (list_item != NULL) {
            TypeDescriptor          *ilType;
            fprintf(stderr, "	Types number %d\n", ++count);
            ilType = (TypeDescriptor *) xanList_getData(list_item);
            assert(ilType != NULL);
            fprintf(stderr, "		Name space	= %s\n", ilType->getTypeNameSpace(ilType));
            fprintf(stderr, "		Type		= %s\n", ilType->getName(ilType));
            list_item = list_item->next;
        }
    }
    xanList_destroyList(ilTypes);
    fprintf(stderr, "\n");

    /* Total time				*/
    fprintf(stderr, "Total time:\n");
    fprintf(stderr, "	CPU Time	= %.6f seconds\n", (system->profiler).total_time);
    fprintf(stderr, "	Wall Time	= %.6f seconds\n", (system->profiler).wallTime.total_time);
    fprintf(stderr, "\n");

    /* Compilation overhead                 */
    if ((system->IRVM).behavior.aot) {
        temp2 = (system->profiler).trampolines_time;
        temp2Wall = (system->profiler).wallTime.trampolines_time;
    } else {
        temp2 = (system->profiler).trampolines_time + (system->profiler).bootstrap_time + (system->profiler).cil_loading_time;
        temp2Wall = (system->profiler).wallTime.trampolines_time + (system->profiler).wallTime.bootstrap_time + (system->profiler).wallTime.cil_loading_time;
    }
    fprintf(stderr, "Compilation overhead:\n");
    internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

    /* Execution time				*/
    if ((system->IRVM).behavior.aot) {
        temp2 = (system->profiler).execution_time;
        temp2Wall = (system->profiler).wallTime.execution_time;
    } else {
        temp2 = (system->profiler).execution_time - (system->profiler).trampolines_time;
        temp2Wall = (system->profiler).wallTime.execution_time - (system->profiler).wallTime.trampolines_time;
    }
    fprintf(stderr, "Time spent executing the machine code generated:\n");
    internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

    if ((system->IRVM).behavior.profiler >= 2) {

        /* System bootstrap			*/
        temp2 = (system->profiler).bootstrap_time;
        temp2Wall = (system->profiler).wallTime.bootstrap_time;
        fprintf(stderr, "System bootstrap:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* CIL loading				*/
        temp2 = (system->profiler).cil_loading_time;
        temp2Wall = (system->profiler).wallTime.cil_loading_time;
        fprintf(stderr, "CIL loading:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* CIL->IR translation			*/
        temp2 = (system->profiler).cil_ir_translation_time;
        temp2Wall = (system->profiler).wallTime.cil_ir_translation_time;
        fprintf(stderr, "CIL->IR translation:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* DLA                                  */
        temp2 = (system->profiler).dla_time;
        temp2Wall = (system->profiler).wallTime.dla_time;
        fprintf(stderr, "DLA:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* IR optimization			*/
        temp2 = (system->profiler).ir_optimization_time;
        temp2Wall = (system->profiler).wallTime.ir_optimization_time;
        fprintf(stderr, "IR optimization:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* IR->Machine code                     */
        temp2 = (system->profiler).ir_machine_code_translation_time;
        temp2Wall = (system->profiler).wallTime.ir_machine_code_translation_time;
        fprintf(stderr, "IR->Machine code translation:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* Static memory				*/
        temp2 = (system->profiler).static_memory_time;
        temp2Wall = (system->profiler).wallTime.static_memory_time;
        fprintf(stderr, "Time spent for static memory initialization:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* Trampolines time			*/
        temp2 = (system->profiler).trampolines_time;
        temp2Wall = (system->profiler).wallTime.trampolines_time;
        fprintf(stderr, "Time spent in trampolines:\n");
        internal_print_cpu_time_and_wall_time(temp2, temp1, temp2Wall, totalWallTime);

        /* Garbage collector			*/
        fprintf(stderr, " Garbage collector time\n");
        fprintf(stderr, "    Total collection time         = %.6f seconds\n", gcInfo.totalCollectTime);
        fprintf(stderr, "    Total allocation time         = %.6f seconds\n", gcInfo.totalAllocTime);
        fprintf(stderr, " Memory\n");
        fprintf(stderr, "    Max Heap used        = %u Bytes  (%.6f KBytes   %.6f MBytes)\n", gcInfo.maxHeapMemory, ((JITFLOAT32) gcInfo.maxHeapMemory) / ((JITFLOAT32) 1024), ((JITFLOAT32) gcInfo.maxHeapMemory) / ((JITFLOAT32) 1048576));
        fprintf(stderr, "    Actual Heap used     = %u Bytes  (%.6f KBytes   %.6f MBytes)\n", gcInfo.actualHeapMemory, ((JITFLOAT32) gcInfo.actualHeapMemory) / ((JITFLOAT32) 1024), ((JITFLOAT32) gcInfo.actualHeapMemory) / ((JITFLOAT32) 1048576));
        fprintf(stderr, "    Heap allocated       = %u Bytes  (%.6f KBytes   %.6f MBytes)\n", gcInfo.heapMemoryAllocated, ((JITFLOAT32) gcInfo.heapMemoryAllocated) / ((JITFLOAT32) 1024), ((JITFLOAT32) gcInfo.heapMemoryAllocated) / ((JITFLOAT32) 1048576));
        fprintf(stderr, "    Over head of the GC used   = %u Bytes  (%.6f KBytes   %.6f MBytes)\n", gcInfo.overHead, ((JITFLOAT32) gcInfo.overHead) / ((JITFLOAT32) 1024), ((JITFLOAT32) gcInfo.overHead) / ((JITFLOAT32) 1048576));

    }

    /* Return				*/
    return;
}

static inline void internal_print_cpu_time_and_wall_time (JITFLOAT32 CPUTime, JITFLOAT32 totalCPUTime, JITFLOAT32 wallTime, JITFLOAT32 totalWallTime) {
    fprintf(stderr, "		CPU Time	= %.6f seconds\n", CPUTime);
    fprintf(stderr, "		CPU Time percent: %.6f\n", (CPUTime * 100) / totalCPUTime);
    fprintf(stderr, "		Wall Time	= %.6f seconds\n", wallTime);
    fprintf(stderr, "		Wall Time percent: %.6f\n", (wallTime * 100) / totalWallTime);
}
