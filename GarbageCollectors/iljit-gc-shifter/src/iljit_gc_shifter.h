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
/**
 *@file iljit_ecmasoft_decoder.h
 */

#ifndef ILJIT_GC_SHIFTER_H
#define ILJIT_GC_SHIFTER_H
#define _GNU_SOURCE

// My headers
#include <iljit-utils.h>
#include <jitsystem.h>
#include <garbage_collector.h>
#include <memory_manager.h>
// End

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

typedef struct {
	JITFLOAT32	collectTime;
	JITFLOAT32	virtualMethodsTime;
	JITFLOAT32	allocTime;
	JITFLOAT32	fieldsTime;
} t_gc_time;

JITINT16		init			(t_gc_behavior *gcBehavior, JITNUINT sizeMemory);
JITINT16		gc_shutdown 		(void);
JITINT32		shifter_getSupport	(void);
void *			gc_allocObject 		(JITNUINT size);
void 			shifter_reallocObject 	(void *obj, JITUINT32 newSize);
void			collect			(void);
void 			shifter_threadCreate (pthread_t *thread, pthread_attr_t *attr, void *(*startRouting)(void *), void *arg);
void			gcInformations		(t_gc_informations *gcInfo);
char *			shifter_getName		(void);
char *			shifter_getVersion	(void);
char *			shifter_getAuthor	(void);

t_memory	memory;
t_gc_time	gcTime;
t_gc_behavior	behavior;
sem_t		gcMutex;

#endif
