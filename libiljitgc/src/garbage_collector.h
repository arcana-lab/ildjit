/*
 * Copyright (C) 2006 - 2011  Campanoni Simone
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
 * @file garbage_collector.h
 *
 * Garbage collector interface
 *
 * This file specify the interface between the garbage collector and the core of
 * the virtual machine. In this file you can found data structures to get info
 * about objects, send info to virtual machine (e.g. heap statistics) and an
 * interface that your garbage collector must implement. The most important
 * things about objects is their structure. An object is divided into two
 * section. The first is object header, the second is object body. The two
 * sections are contigus. All objects addresses that we use points to object
 * body, so if your garbage collector want to move an object remeber to move
 * also the header. The header starts at object - HEADER_FIXED_SIZE. Good luck!
 */
#ifndef GARBAGE_COLLECTOR_H
#define GARBAGE_COLLECTOR_H

/* These headers includes all iljit public headers */
#include <jitsystem.h>
#include <xanlib.h>

/* Include some headers for profile macros */
#include <stdio.h>
#include <time.h>

/**
 * My garbage collector uses rootset
 *
 * Return this constant in your getSupport implementation if you want to access
 * system root set.
 */
#define ILDJIT_GCSUPPORT_ROOTSET                1

/**
 * My garbage collector creates thread
 *
 * Return this constant in your getSupport implementation if you want to manage
 * thread creation for CIL code.
 */
#define ILDJIT_GCSUPPORT_THREAD_CREATION        2

/**
 * Object monitor size
 */
#ifdef USE_LIB_LOCK_OS
#define MONITOR_SIZE                            80
#elif defined USE_LIB_LOCK_THIN
#define MONITOR_SIZE                            4
#else
#define MONITOR_SIZE                            80
#endif

/**
 * Object header size
 */
//#define HEADER_FIXED_SIZE			((sizeof(void *) * 3) + sizeof(JITUINT8) + sizeof(JITUINT16) + (sizeof(JITUINT32) * 2) + MONITOR_SIZE)
#define HEADER_FIXED_SIZE 144

/**
 * Print a start collection debug message
 *
 * Call this macro before each collection to print a special message. This
 * message can be interpreted by a profiling tool.
 *
 * @param name		garbage collector name
 * @param verbose	if JITTRUE print debug message
 */
#define GC_START_COLLECTION(name, verbose)			\
	if (verbose)						 \
	{							\
		time_t gc_start_collection_time;	       \
								\
		time(&gc_start_collection_time);		\
		printf("GC: %s: Start collection at %s",	\
		       name,					\
		       ctime(&gc_start_collection_time));	\
	}

/**
 * Print a stop collection debug message
 *
 * Call this macro after each collection to print a special message. This
 * message can be interpreted by a profiling tool.
 *
 * @param name		garbage collector name
 * @param verbose	if JITTRUE print debug message
 */
#define GC_END_COLLECTION(name, verbose)			\
	if (verbose)						 \
	{							\
		time_t gc_end_collection_time;		       \
								\
		time(&gc_end_collection_time);			\
		printf("GC: %s: End collection at %s",		\
		       name,					\
		       ctime(&gc_end_collection_time));		\
	}

/**
 * Garbage collector profile information
 *
 * Profile informations of the execution of the garbage collector (GC).
 */
typedef struct {

    /**
     * Currently heap size
     *
     * Size of the heap allocated by the garbage collector to store the
     * objects.
     */
    JITUINT32 heapMemoryAllocated;

    /**
     * Currently heap used bytes
     */
    JITUINT32 actualHeapMemory;

    /**
     * Garbage collector overhead memory
     *
     * Bytes of memory used by the garbage collector to make its task.
     */
    JITUINT32 overHead;

    /**
     * Maximum of actualHeapMemory
     *
     * Max number of bytes of the heap used for the object between two
     * collections.
     */
    JITUINT32 maxHeapMemory;

    /**
     * Total collection time
     *
     * Seconds spent by the garbage collector to collect the memory.
     */
    JITFLOAT32 totalCollectTime;

    /**
     * Total allocation time
     *
     * Seconds spent by the garbage collector to allocate objects.
     */
    JITFLOAT32 totalAllocTime;
} t_gc_informations;

/**
 * Behavior specification of the garbage collector
 *
 * Structure given as input to the GC during the ILDJIT initialization phase.
 * Each garbage collector can use the method specified by this structure.
 */
typedef struct {

    /**
     * Profiling mode setting
     *
     * This flag is set by the runtime engine to enable the storing of the
     * profile informations.
     */
    JITBOOLEAN profile;

    /**
     * Verbose mode setting
     *
     * This flag is set by the runtime engine to enable the verbose mode of
     * the GC.
     */
    JITBOOLEAN verbose;

    /**
     * Collectable test function
     *
     * This function is callable by the garbage collector (GC) to know if
     * an object is collectable or not.
     *
     * @param object	the object to check
     *
     * @return              JITTRUE if object is collectable, JITFALSE
     *                      otherwise
     */
    JITBOOLEAN (*isCollectable)(void* object);

    /**
     * Get given object size
     *
     * This function is callable by the GC to know the number of bytes of
     * the object given as input.
     *
     * @param object	the object with unknown size
     *
     * @return		given object size, in bytes
     */
    JITNUINT (*sizeObject)(void* object);

    /**
     * Get the list of reference fields of given object
     *
     * Return a list of void*. Each list element identify a reference field in
     * given object. The garbage collector has to free it after usage.
     *
     * @param object	an object or an array
     *
     * @return              a list of addresses
     *
     * @note May change
     */
    XanList*        (*getReferencedObject)(void *object);

    /**
     * Get current root set
     *
     * Return a list of void*. Each returned list element is a root reference
     * address. The garbage collector cannot free the list after its usage,
     * iljit has to do it. Note that a root set element can point to a never
     * allocated object and the garbage collector has to manage this
     * situation ignoring these elements.
     *
     * @return		the current root set
     *
     * @note May change
     */
    XanList*        (*getRootSet)(void);

    /**
     * Call given object finalizer
     *
     * Call "Finalize" method on given object. After this call object can be
     * safely collected.
     *
     * @param object	the object to "finalize"
     */
    void (*callFinalizer)(void* object);
} t_gc_behavior;

/**
 * Garbage collector methods
 *
 * Functions that the GC has to implement. To build a working plugin remeber to
 * export a plugin_interface symbol of t_garbage_collector_plugin type.
 */
typedef struct {

    /**
     * Initialize the GC. If sizeMemory is set to zero, then the garbage
     * collector is free to choose the heap size automatically.
     *
     * @param behavior	setup info and functions callbacks
     * @param sizeMemory	the requested heap size
     */
    void (*init)(t_gc_behavior* behavior, JITNUINT sizeMemory);

    /**
     * Shutdown the GC
     */
    JITINT16 (*shutdown)(void);

    /**
     * Return needed support
     *
     * Return the ID of each support job that the garbage collector ask to
     * the ILDJIT system.
     *
     * @return needed ILDJIT support
     */
    JITINT32 (*getSupport)(void);

    /**
     * Allocation function
     *
     * This function alloc memory inside the heap for a new object with
     * size bytes.
     *
     * @param size	the memory amount to alloc
     *
     * @return	a pointer to new allocated memory or NULL if allocation
     *              fails
     */
    void* (*allocObject)(JITNUINT size);

    /**
     * Allocation function
     *
     * This function free the memory pointed by obj.
     *
     * @param obj Object to free
     */
    void (*freeObject)(void *obj);

    /**
     * Run the collection algorithm
     */
    void (*collect)(void);

    /**
     * Create an user thread
     *
     * This function will be called by virtual machine every time a CIL
     * program needs a new thread. If you don't support this feature simply
     * implement this function with a call to abort.
     *
     * @param thread	new thread identifier
     * @param attr		thread attributes
     * @param startRoutine	thread main function
     * @param arg		thread main function arguments
     */
    void (*threadCreate)(pthread_t* thread,
                         pthread_attr_t* attr,
                         void* (*startRoutine)(void*),
                         void* arg);

    /**
     * Get GC informations
     *
     * @param gcInfo an info structure to fill in
     */
    void (*gcInformations)(t_gc_informations* gcInfo);

    /**
     * Get the name of the GC
     *
     * @return	GC name
     */
    char*           (*getName)(void);

    /**
     * Get the version of the GC
     *
     * @return	GC version
     */
    char*           (*getVersion)(void);

    /**
     * Get the author name of the GC
     *
     * @return GC author
     */
    char*           (*getAuthor)(void);

    /**
     * Get the compilation flags
     */
    void (* getCompilationFlags)(char *buffer, JITINT32 bufferLength);

    /**
     * Get the time on when the plugin has been compiled
     */
    void (* getCompilationTime)(char *buffer, JITINT32 bufferLength);
} t_garbage_collector_plugin;

#endif /* GARBAGE_COLLECTOR_H */
