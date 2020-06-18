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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <gc/gc.h>

// My header
#include <iljit_gc_bdw.h>
#include <system.h>
// End

t_garbage_collector_plugin plugin_interface={
	bdw_init			,
	bdw_gc_shutdown			,
	bdw_getSupport			,
	bdw_allocObject			,
	bdw_reallocObject		,
	bdw_collect			,
	bdw_threadCreate		,
	bdw_gcInformations		,
	bdw_getName			,
	bdw_getVersion			,
	bdw_getAuthor
};

JITINT16 bdw_init (t_gc_behavior *gcBehavior, JITNUINT sizeMemory){
		
	/* Assertions			*/
	assert(gcBehavior != NULL);
	PDEBUG("GC BDW: init: Start\n");

	/* Store the behavior		*/
	memcpy(&behavior, gcBehavior, sizeof(t_gc_behavior));

	GC_INIT();
	if (sizeMemory > 0){
		GC_set_max_heap_size((size_t)sizeMemory);
	}

	/* Initialize the time 		*
	 * informations			*/
	gcTime.collectTime		= 0;
	gcTime.allocTime		= 0;

	/* Return			*/
	PDEBUG("GC BDW: init: Exit\n");
	return 0;
}

JITINT16 bdw_gc_shutdown (void){
	PDEBUG("GC BDW: shutdown: Start\n");
	PDEBUG("GC BDW: shutdown: Exit\n");
	return 0;
}

void * bdw_allocObject (JITNUINT size){
	void			*free_memory;
	struct timespec		startTime;
	struct timespec		stopTime;

	/* Assertions			*/
	assert(size > 0);
	PDEBUG("GC BDW: allocObject: Start\n");
	
	/* Fetch the start time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &startTime);
	}

	/* Initialize the variable	*/
	free_memory		= NULL;
	
	/* Print the layout of the object	*/
	PDEBUG("GC BDW: allocObject:	Object size	 	= %d Bytes\n", size);

	/* Search a memory block		*/
	free_memory	= GC_MALLOC(size);

	/* The free_memory pointer, points to 	*
	 * the first byte of the memory block	*/
	if (free_memory != NULL){

		/* Initialize the fields of the object	*/
		memset(((void *)free_memory), 0, size);
	} else {
		PDEBUG("GC BDW: allocObject:	Memory full\n");
	}

	/* Fetch the stop time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &stopTime);
	
		/* Store profiling informations		*/
		gcTime.allocTime	+= diff_time(startTime, stopTime);
	}

	/* Return the new object	*/
	PDEBUG("GC BDW: allocObject: Exit\n");
	return free_memory;
}

void bdw_collect (void){
	struct timespec		startTime;
	struct timespec		stopTime;
	
	PDEBUG("GC BDW: collect: Start\n");
	
	/* Fetch the start time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &startTime);
	}

	/* Collect the garbage memory	*/
	PDEBUG("GC BDW: collect: 	Run the collection\n");
	GC_gcollect();

	/* Fetch the stop time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &stopTime);
	
		/* Store profiling informations		*/
		gcTime.collectTime	+= diff_time(startTime, stopTime);
	}

	/* Return			*/
	PDEBUG("GC BDW: collect: Exit\n");
	return;
}

void bdw_gcInformations (t_gc_informations *gcInfo){

	/* Assertions			*/
	assert(gcInfo != NULL);

	/* Store the time informations	*/
	gcInfo->totalCollectTime	= gcTime.collectTime;
	gcInfo->totalAllocTime		= gcTime.allocTime;

	/* Return			*/
	return;
}

void bdw_threadCreate (pthread_t *thread, pthread_attr_t *attr, void *(*startRouting)(void *), void *arg){

	/* Assertions				*/
	assert(thread != NULL);
	assert(startRouting != NULL);
	GC_pthread_create(thread, attr, startRouting, arg);
}

char * bdw_getName (void){
	return "Bdw";
}

char * bdw_getVersion (void){
	return VERSION;
}

char * bdw_getAuthor (void){
	return "Boehm-Demers-Weiser";
}

JITINT32 bdw_getSupport (void){
	return ILDJIT_GCSUPPORT_THREAD_CREATION;
}

void bdw_reallocObject (void *obj, JITUINT32 newSize){
	void	*newObj;
	
	newObj	= GC_REALLOC(obj, newSize);
	assert(newObj == obj);
}
