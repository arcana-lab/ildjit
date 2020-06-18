
#ifndef ILJIT_GC_COPY_H
#define ILJIT_GC_COPY_H

/* For basic type declarations */
#include <jitsystem.h>

/* For garbage collector interface */
#include <garbage_collector.h>

/* For gc_memory_t */
#include <gc_memory.h>

/**
 * @mainpage
 *
 * @section intro Introduction
 *
 * This document contains info about iljit-gc-copy garbage collector. It is an
 * extension of iljit project that implements a simple copy garbage collector
 * algorithm.
 *
 * @section iljit-intro What is iljit?
 *
 * iljit is a jit in time compiler for CIL language developed at Politecnico di
 * Milano. It allows to be customized by adding several optimization plugin and
 * garbage collectors. More info can be founded on iljit web site
 * (http://ildjit.sourceforge.net).
 *
 * @section garbage-collector Garbage collector
 *
 * A garbage collector is a piece of software that manage a program heap. It
 * allows to allocate memory in the heap and to implicity free previous
 * allocated memory. This allows programmers to focus on bussiness logic and
 * to forget to explicity release memory. In copy garbage collectors memory
 * deallocation is performed by dividing object into two classes: live and dead.
 *
 * An object is live if it is reachable by the user program. Otherwise the
 * object is dead. For example consider the following code fragment:
 *
 * @code
 *
 * a = new Object();
 * a = null;
 *
 * @endcode
 *
 * The first instruction alloc on the heap a new object. The a variable can be
 * used to access to the object, so after this instruction the object is live.
 * The second instruction change a referenced object. Because a was the only
 * reference to the object now it isn't reachable from user program, so it is
 * dead. Dead object are useless so we need to remove them from heap.
 *
 * All copy collectors divide process heap into zones called semispaces. In the
 * most simple case, our case, we use two semispaces. One is called online
 * semispace the other offline semispace. During its life a semispace can
 * change its role, e.g. becoming offline if it was online, but there is always
 * only one online semispace and one offline semispace. A semispace can't be
 * online and offline at same time. During normal operation when user program
 * wants memory the garbage collector find memory in online semispace. If the
 * online semispace can't give enougth memory we need to start a garbage
 * collection. The user process is stopped (stop and wait behavior) until
 * collection is done. After collection we try to alloc the wanted memory. If
 * this last change fails we inform user program that we have finished the
 * memory. Note that during normal operation (memory allocation) offline
 * semispace is useless.
 *
 * @subsection collection-algorithm Collection algorithm
 *
 * To find garbage we don't find dead object. We search any live object in
 * online semispace and move they on offline semispace. After this step all
 * live object are stored in offline semispace and all dead objects are inside
 * online semispace. To complete collection process we swap semispace roles.
 *
 * To run this algorithm we need to model the objects relations. We use a
 * digraph where nodes are objects and relations are references beetween
 * objects. By visit all nodes in the graph we can reach any live object.
 * Dead objects are ones that aren't reachable from the graph visit starting
 * point. The starting point is a set of objects reachable from program root
 * references (root set). The root set usually contains references that are
 * stored on process stack, such as functions parameters or global variables.
 * The graph is traversed with a deep first visit to improve objects spatial
 * locality.
 * 
 * @section performance Performance
 *
 * This garbage collector is very fast during normal memory allocation because
 * in this phase it only needs to update a pointer. It also reduce memory
 * fragmentation and so improve memory access time. Bad novels comes when we
 * need to run collection algorithm. We need to stop program execution and
 * visit objects graph. If in the graph there are a large number of live objects
 * we can take a huge time to perform collection. This is the case of GCBench
 * benchmark. This benchmark build a big tree in memory and if iljit compiler
 * is launched with less than 50 MB heap size we need to perform collection
 * when the tree is still live. The collection process takes a very long time.
 *
 * During project development we have tryed to improve the collection algorithm
 * performace by using a stack insted of function recursion to speedup graph
 * visit. This improvment has enanched performance but GCBench is still
 * unpassable in acceptable time.
 *
 * This kind of problem is very common on garbage collectors that need to
 * traverse an objects graphs and use stop and wait behavior. At this time to
 * bypass the problem we need to use an heap with an ad-hoc size. Some
 * improvement can be implemented to remove this limit:
 *
 *   - using a pool of threads to visit object graphs
 *   - allow garbage collector to dynamic increase heap size
 *
 * The first solution can be used on multicore or multiprocessor system, but it 
 * is an improvement like stack recursion. It is a technology improvement and
 * if we alloc a lot of objects in the heap this solution is still slow.
 *
 * The second solution must use a sort of heuristic to chose what to do when
 * the online semispace is full: running collection algorithm or increase heap
 * size? If in the heap there aren't many objects the cheaper solution is
 * running collection algorithm, otherwise it's better to increase heap size.
 * Here the problem is select a good heuristic function.
 */

/**
 * Returned by a routine when no error occours
 */
#define GC_NO_ERROR 0

/**
 * Wrong interface given to garbage collector
 */
#define GC_BAD_IFACE 1

/**
 * Garbage collector name
 */
#define GC_NAME "Copy"

/**
 * Garbage collector version
 *
 * @todo add a $Id SVN tag
 */
#define GC_VERSION PACKAGE_VERSION

/**
 * Garbage collector author
 */
#define GC_AUTHOR "Speziale Ettore"

/**
 * Default heap size (1MB)
 */
#define GC_DEFAULT_HEAP_SIZE 1024 * 1024;

/**
 * Managed memory
 */
extern gc_memory_t heap;

/**
 * Get the size of given object
 *
 * @param object the object whose size is unknown
 *
 * @return the size, in bytes, of given object
 */
extern JITNUINT (*object_size)(void* object);

/**
 * Get all children of an object
 *
 * Call this function to get a list of reference stored in given object. From
 * reference you can access to an object reachable by the given one.
 *
 * @param object the 'father' object
 *
 * @return a list of void**
 */
extern XanList* (*object_children)(void* object);

/**
 * Get root set
 *
 * Call a ildjit compiler routine to access current root set. A list of void**
 * is returned. Don't free returned list afrer use.
 *
 * @return a list of reference pointers
 */
extern XanList* (*get_root_set)(void);

/**
 * Initialize garbage collector
 *
 * Setup garbage collector environment. Returns GC_BAD_IFACE if some field of
 * behavior are wrong. GC_NO_ERROR otherwise.
 *
 * @param behavior    a set of parameter from ildjit compiler
 * @param memory_size the size of heap to manage
 *
 * @return GC_NO_ERROR or GC_BAD_IFACE if behavior is wrong
 */
JITINT16 gc_init(t_gc_behavior* behavior, JITNUINT memory_size);

/**
 * Turn off garbage collector
 *
 * Stop garbage collector and release all resources.
 *
 * @return always GC_NO_ERROR
 */
JITINT16 gc_shutdown(void);

/**
 * Return mandatory features
 *
 * This copy collector needs root set to work. This is declared by this function returning
 * ILDJIT_GCSUPPORT_ROOTSET.
 *
 * @return ILDJIT_GCSUPPORT_ROOTSET
 */
JITINT32 gc_get_support(void);

/**
 * Alloc a new memory segment
 *
 * Alloc size bytes in the heap and return a pointer to it. If there aren't
 * enought memory or size is 0 NULL is returned.
 *
 * @param size the number of bytes to alloc
 *
 * @return a pointer to allocated memory or NULL if something goes wrong
 *
 * @note if there aren't enought memory i must run the collection algorithm?
 */
void* gc_alloc_object(JITNUINT size);

/**
 * Realloc an object with a new size
 *
 * This function isn't yet implemented.
 *
 * @param object the object to realloc
 * @param size the new size of the object
 *
 * @return the object new address
 */
void* gc_realloc_object(void* object, JITUINT32 size);

/**
 * Run collection algorithm
 */
void gc_collect(void);

/**
 * Build a new thread for CIL program
 *
 * This capability isn't supported by this collector: this function will abort
 * if called.
 */
void gc_threadCreate(pthread_t* thread,
		     pthread_attr_t* attr,
		     void* (*startRoutine)(void*),
		     void* arg);
/**
 * Get garbage collector statistics
 *
 * @param info a pointer to a structure that holds info
 */
void gc_informations(t_gc_informations *info);

/**
 * Get garbage collector name
 *
 * @return garbage collector name
 */
JITINT8* gc_get_name(void);

/**
 * Get garbage collector version
 *
 * @return garbage collector version
 */
JITINT8* gc_get_version(void);

/**
 * Get garbage collector author
 *
 * @return garbage collector author
 */
JITINT8* gc_get_author(void);

/**
 * Garbage collector interface
 *
 * This symbol is searched by ildjit compiler to use garbage collector
 */
extern t_garbage_collector_plugin plugin_interface;

#endif /* ILJIT_GC_COPY_H */
