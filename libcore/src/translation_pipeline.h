/*
 * Copyright (C) 2006-2009      Simone Campanoni <simo.xan@gmail.com>
 *
 * Compilation pipeline system. In this module the compilation tasks are dispatched and performed in parallel.
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
#ifndef TRANSLATIONPIPELINE_H
#define TRANSLATIONPIPELINE_H

#include <platform_API.h>
#include <jitsystem.h>
#include <ilmethod.h>

#define MAX_METHOD_PRIORITY                     1.0f
#define MIN_METHOD_PRIORITY                     0.0f

#define MAX_CILIR_LOW_THREADS                       1
#define MAX_DLA_LOW_THREADS                         1
#define MAX_IROPTIMIZER_LOW_THREADS                 1
#define MAX_IRMACHINECODE_LOW_THREADS               1
#define START_CCTOR_LOW_THREADS                     1

#define MAX_CILIR_HIGH_THREADS                       1
#define MAX_DLA_HIGH_THREADS                         1
#define MAX_IROPTIMIZER_HIGH_THREADS                 1
#define MAX_IRMACHINECODE_HIGH_THREADS               1
#define START_CCTOR_HIGH_THREADS                     1

typedef struct {
    XanQueue *pipe;
    XanQueue *busyWorkers;

    JITUINT32 steadyThreadsCount;

    pthread_t *threads;
    JITUINT32 threadsCount;

    pthread_mutex_t mutex;
    pthread_cond_t wakeupCondition;
} workerSet_t;

typedef struct {
    pthread_mutex_t profileMutex;
    workerSet_t highPriority;
    workerSet_t lowPriority;

} ildjitPipe_t;

typedef struct {
    XanQueue         *pipe;
    XanHashTable    *methodsInScheduling;

    pthread_t thread;
    pthread_cond_t wakeupCondition;
    pthread_mutex_t mutex;
} schedPipe_t;

typedef struct TranslationPipeline {
    ildjitPipe_t cilPipe;
    ildjitPipe_t dlaPipe;
    ildjitPipe_t irOptimizerPipe;
    ildjitPipe_t irPipe;
    ildjitPipe_t cctorPipe;
    schedPipe_t schedulerPipe;
    pthread_mutex_t mutex;
    pthread_cond_t emptyCondition;
    XanHashTable    *methodsInPipe;

    pthread_t waitPipelineThread;

    JITUINT32 steady_cctor_threads;                 /**< Number of cctor threads that are steady									*/

    JITUINT32 rescheduledTask;
    JITUINT32 methodsCompiled;                      /**< Number of methods compiled.										*/
    JITUINT32 methodsMovedToHighPriority;           /**< Number of methods which their priority have been changed after its insertion to the compilation pipeline.  */
    JITUINT32 reprioritizedTicket;
    JITUINT32 methodsCompiledInHighPriority;        /**< Number of methods compiled using the highest priority.                                                     *
   	                                                  *  These methods are compiled from the begin of the compilation using the high priority.                      */
    JITBOOLEAN running;				/**< JITTRUE if the pipeline is running; JITFALSE otherwise							*/

    void		(*startPipeliner)		(struct TranslationPipeline *_this);
    void		(*synchInsertMethod)		(struct TranslationPipeline *_this, Method method, JITFLOAT32 priority);
    void		(*synchTillIRMethod)		(struct TranslationPipeline *_this, Method method, JITFLOAT32 priority);
    JITINT32	(*insertMethod)			(struct TranslationPipeline *_this, Method method, JITFLOAT32 priority);
    JITINT16	(*isInPipe)			(struct TranslationPipeline *_this, Method method);
    void		(*waitEmptyPipeline)		(struct TranslationPipeline *_this);
    JITBOOLEAN	(*isCCTORThread)		(struct TranslationPipeline *_this, pthread_t thread);
    void		(*destroyThePipeline)		(struct TranslationPipeline *_this);
    void		(*declareWaitingCCTOR)		(struct TranslationPipeline *_this);
    void		(*declareWaitingCCTORDone)	(struct TranslationPipeline *_this);
} TranslationPipeline;

void init_pipeliner (TranslationPipeline *pipeliner);

#endif
