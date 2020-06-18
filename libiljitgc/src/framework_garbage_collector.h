/*
 * Copyright (C) 2011  Campanoni Simone
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
#ifndef FRAMEWORK_GARBAGE_COLLECTOR_H
#define FRAMEWORK_GARBAGE_COLLECTOR_H

#include <jitsystem.h>
#include <metadata/metadata_types.h>
#include <garbage_collector.h>

typedef struct {
    JITBOOLEAN useIMT;
    JITUINT32 vtable_index;
} VCallPath;

/**
 * Garbage collector interface.
 */
typedef struct {
    t_garbage_collector_plugin      *gc_plugin;     /**< GC Interface					*/

    void 		(*init)(t_gc_behavior *behavior, JITNUINT sizeMemory);
    JITINT16 	(*shutdown)(void);
    JITINT32 	(*getSupport)(void);
    void 		(*collect)(void);
    void 		(*gcInformations)(t_gc_informations *gcInfo);

    /* Root sets				*/
    void 		(*addRootSet)(void ***rootSet, JITUINT32 size);
    void 		(*popLastRootSet)(void);

    /* Threads				*/
    JITINT32 	(*threadCreate)(pthread_t *thread, pthread_attr_t *attr, void *(*startRouting)(void *), void *arg);
    void 		(*threadExit)(pthread_t thread);

    /* Object allocation			*/
    void *          (*allocPermanentObject)(TypeDescriptor * classDescriptor, JITUINT32 overSize);
    void *          (*allocObject)(TypeDescriptor * classDescriptor, JITUINT32 overSize);
    void *          (*allocStaticObject)(TypeDescriptor * classDescriptor, JITUINT32 overSize);
    void *          (*allocArray)(TypeDescriptor * classDescriptor, JITINT32 rank, JITINT32 size);
    void *          (*createArray)(ArrayDescriptor * array, JITINT32 size);
    void 		(*freeObject)(void *obj);

    /* Object type				*/
    TypeDescriptor *(*getType)(void * object);
    JITINT32 	(*getTypeSize)(void * object);
    JITBOOLEAN 	(*isSubtype)(void * object, TypeDescriptor *classDescriptor);

    /* Object fields			*/
    JITINT32 	(*fetchTypeDescriptorOffset)(void);
    JITINT32 	(*fetchFieldOffset)(FieldDescriptor *field);
    JITINT32 	(*fetchOverSizeOffset)(void * object);
    JITINT32 	(*fetchStaticFieldOffset)(FieldDescriptor *field);	/**< This function return the offset from the object pointer where is stored the specific field identified by the fieldID parameter. It is called during the translation from IR to the JIT language	*/
    JITINT32 	(*fetchVTableOffset)(void); /**< This function return the offset from the object pointer where is stored the specific field identified by the fieldID parameter. It is called during the translation from IR to the JIT language	*/
    JITINT32 	(*fetchIMTOffset)(void);
    VCallPath 	(*getIndexOfVTable)(MethodDescriptor *methodID);	/**< This function fill up inside the vmethod parameter, the method ID (MethodDescriptor) of the virtual method identified by the method parameter and the binary where the right method is stored. It is called during the execution of the CIL program	*/
    JITNUINT 	(*getSize)(void * object);
    void **         (*getVTable)(void * object);

    /* Array				*/
    JITUINT32 	(*getArrayLength)(void * array);
    JITUINT16 	(*getArrayRank)(void * array);
    JITINT32 	(*getArrayLengthOffset)(void);
    JITUINT32 	(*getArraySlotIRType)(void * array);

    /* Objects flags			*/
    void 		(*setStaticFlag)(void * object);
    void 		(*unsetStaticFlag)(void * object);
    void 		(*setPermanentFlag)(void * object);
    void 		(*unsetPermanentFlag)(void * object);
    void 		(*setFinalizedFlag)(void * object);

    /**
     * Lock given object
     *
     * @param object the object to unlock
     * @param timeout milliseconds to wait for lock acquisition. If -1 wait
     *                lock has been acquired, if 0 don't wait
     *
     * @return JITTRUE if lock was acquired, JITFALSE otherwise
     */
    JITBOOLEAN (*lockObject)(void* object, JITINT32 timeout);

    /**
     * Unlock given object
     *
     * @param object the object to unlock
     */
    void (*unlockObject)(void* object);

    /**
     * Wait for a pulse signal on given object
     *
     * @param object the object where wait for the signal
     * @param timeout milliseconds to wait for the signal. If -1 wait until
     *                signal occurs, if 0 don't wait
     *
     * @return JITTRUE if pulse occurs before timeout, JITFALSE otherwise
     */
    JITBOOLEAN (*waitForPulse)(void* object, JITINT32 timeout);

    /**
     * Send a pulse signal on object
     *
     * @param object where send the pulse signal
     */
    void (*signalPulse)(void* object);

    /**
     * Send a pulse all signal on object
     *
     * @param object where send the pulse signal
     */
    void (*signalPulseAll)(void* object);
} t_running_garbage_collector;

TypeDescriptor * GC_getObjectType (void* object);

#endif
