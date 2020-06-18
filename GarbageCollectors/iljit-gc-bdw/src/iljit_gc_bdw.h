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

#include <pthread.h>
#include <jitsystem.h>
#include <iljit-utils.h>
#include <garbage_collector.h>

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

typedef struct {
	JITFLOAT32	collectTime;
	JITFLOAT32	allocTime;
} t_gc_time;

JITINT16		bdw_init			(t_gc_behavior *gcBehavior, JITNUINT sizeMemory);
JITINT16		bdw_gc_shutdown 		(void);
JITINT32		bdw_getSupport			(void);
void *			bdw_allocObject 		(JITNUINT size);
void			bdw_reallocObject 		(void *obj, JITUINT32 newSize);
void			bdw_collect			(void);
void	 		bdw_threadCreate (pthread_t *thread, pthread_attr_t *attr, void *(*startRouting)(void *), void *arg);
void			bdw_gcInformations		(t_gc_informations *gcInfo);
char *			bdw_getName			(void);
char *			bdw_getVersion			(void);
char *			bdw_getAuthor			(void);

t_gc_time	gcTime;
t_gc_behavior	behavior;

#endif
