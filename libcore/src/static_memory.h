/*
 * Copyright (C) 2008 - 2011 Campanoni Simone
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
/*
 * @file static_memory.h Static memory manager
 *
 * This file manages the static memory needed by the CIL programs.
 */
#ifndef STATIC_CONSTRUCTOR_H
#define STATIC_CONSTRUCTOR_H

#include <xanlib.h>
#include <jitsystem.h>
#include <platform_API.h>
#include <ilmethod.h>

/**
 *
 * Static memory manager
 */
typedef struct StaticMemoryManager {
    XanHashTable            *staticObjects;         /**< All the static object				*/
    XanHashTable            *cctorMethodsExecuted;
    XanList			*cachedConstructors;
    JITBOOLEAN		cacheConstructors;
    XanHashTable		*symbolTable;
    XanHashTable		*waitMethodSet;		/**< Contains thread waiting for a method translation. The hash table key is a pthread_t*, while the associated element is a Method waited by thread for translation.*/
    XanHashTable		*translatingSet;	/**< Contains thread "translating" methods. The hash table key is a ILMethodID, while the associated element is a pthread_t* "translating" the method. 	*/

    Method (*fetchStaticConstructor)(struct StaticMemoryManager *manager, MethodDescriptor *method);

    /**
     * Notify that cctor thread thread is blocked and waiting for method
     * translation
     *
     * @param self like this pointer in OO languages
     * @param thread the blocked cctor thread
     * @param method the method that blocks computation
     */
    void (*registerCompilationNeeded)(struct StaticMemoryManager* self, pthread_t thread, Method method);

    /**
     * Notify that cctor thread ends waiting for a method translation
     *
     * @param self like this pointer in OO languages
     * @param thread the unblocked cctor thread
     */
    void (*registerCompilationDone)(struct StaticMemoryManager* self, pthread_t thread);

    JITBOOLEAN (*compilationOfMethodLeadsToADeadlock)(struct StaticMemoryManager* self, pthread_t waitingThread, Method method);
} StaticMemoryManager;

void STATICMEMORY_initStaticMemoryManager (StaticMemoryManager *manager);

void * STATICMEMORY_fetchStaticObject (StaticMemoryManager *manager, Method method, TypeDescriptor *type);
ir_symbol_t * STATICMEMORY_fetchStaticObjectSymbol (StaticMemoryManager *manager, Method method, TypeDescriptor *type);

JITINT32 STATICMEMORY_areStaticConstructorsExecutable (XanList *methods);
void STATICMEMORY_callCctorMethod (StaticMemoryManager* self, Method cctor);
void STATICMEMORY_callStaticConstructors (StaticMemoryManager* self, XanList* cctors);

void STATICMEMORY_cacheConstructorsToCall (StaticMemoryManager *manager);
void STATICMEMORY_callCachedConstructors (StaticMemoryManager *manager);
void STATICMEMORY_flushCachedConstructors (StaticMemoryManager *manager);

JITBOOLEAN STATICMEMORY_isThreadTranslatingMethod (StaticMemoryManager* self, pthread_t thread, Method method);

/**
 * Register that thread is "translating" method in cctors stage
 *
 * @param self like this pointer in OO languages
 * @param method the "being translated" method
 * @param thread the "translator" thread
 * @result JITTRUE if was inserted, JITFALSE in case the mapping was already known
 */
JITBOOLEAN STATICMEMORY_registerTranslator (StaticMemoryManager* self, Method method, pthread_t thread);

/**
 * Unregister the "translator" for method
 *
 * @param self like this pointer in OO languages
 * @param method the just "translated" method
 */
void STATICMEMORY_unregisterTranslator (StaticMemoryManager* self, Method method);

/**
 * Make cctors executable
 *
 * Despite the name this method doesn't make all cctors executable. It
 * try to take an initialization lock on a cctor. If successes it will
 * compile the cctor. On failure compiling the cctor isn't a duty of
 * this thread.
 *
 * @param self like this pointer in OO languages
 * @param cctors a list of cctors to compile
 * @param priority the priority of compile job
 */
void STATICMEMORY_makeStaticConstructorsExecutable (StaticMemoryManager* self, XanList* cctors, JITFLOAT32 priority);

void STATICMEMORY_resetStaticMemoryManager (StaticMemoryManager *manager);

void STATICMEMORY_shutdownMemoryManager (StaticMemoryManager *manager);

#endif
