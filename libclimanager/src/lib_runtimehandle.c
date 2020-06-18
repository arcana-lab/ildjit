/*
 * Copyright (C) 2009  Campanoni Simone
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

#include <compiler_memory_manager.h>
#include <ir_virtual_machine.h>
#include <platform_API.h>

/* Module header */
#include <climanager_system.h>
#include <lib_runtimehandle.h>

/* Method implementations to interface casts */
#define RHM_SETRUNTIMEFIELDHANDLEVALUE rhm_setRuntimeFieldHandleValue
#define RHM_SETRUNTIMEMETHODHANDLEVALUE rhm_setRuntimeMethodHandleValue
#define RHM_SETRUNTIMETYPEHANDLEVALUE rhm_setRuntimeTypeHandleValue
#define RHM_SETRUNTIMEARGUMENTHANDLEVALUE rhm_setRuntimeArgumentHandleValue
#define RHM_GETRUNTIMEFIELDHANDLEVALUE  rhm_getRuntimeFieldHandleValue
#define RHM_GETRUNTIMEMETHODHANDLEVALUE  rhm_getRuntimeMethodHandleValue
#define RHM_GETRUNTIMETYPEHANDLEVALUE rhm_getRuntimeTypeHandleValue
#define RHM_GETRUNTIMEARGUMENTHANDLEVALUE  rhm_getRuntimeArgumentHandleValue
#define RHM_GETRUNTIMEFIELDHANDLEVALUEOFFSET  rhm_getRuntimeFieldHandleValueOffset
#define RHM_GETRUNTIMEMETHODHANDLEVALUEOFFSET  rhm_getRuntimeMethodHandleValueOffset
#define RHM_GETRUNTIMETYPEHANDLEVALUEOFFSET rhm_getRuntimeTypeHandleValueOffset
#define RHM_GETRUNTIMEARGUMENTHANDLEVALUEOFFSET  rhm_getRuntimeArgumentHandleValueOffset
#define RHM_FILLILRUNTIMETYPEHANDLE  internal_fillILRuntimeHandle

extern CLIManager_t *cliManager;

/* Public methods prototypes */
static inline void rhm_setRuntimeFieldHandleValue (RuntimeHandleManager* self, void* handle, void* value);
static inline void rhm_setRuntimeMethodHandleValue (RuntimeHandleManager* self, void* handle, void* value);
static inline void rhm_setRuntimeTypeHandleValue (RuntimeHandleManager* self, void* handle, void* value);
static inline void rhm_setRuntimeArgumentHandleValue (RuntimeHandleManager* self, void* handle, void* value);
static inline void* rhm_getRuntimeFieldHandleValue (RuntimeHandleManager* self, void* handle);
static inline void* rhm_getRuntimeMethodHandleValue (RuntimeHandleManager* self, void* handle);
static inline void* rhm_getRuntimeTypeHandleValue (RuntimeHandleManager* self, void* handle);
static inline void* rhm_getRuntimeArgumentHandleValue (RuntimeHandleManager* self, void* handle);
static inline JITINT32 rhm_getRuntimeFieldHandleValueOffset (RuntimeHandleManager* self);
static inline JITINT32 rhm_getRuntimeMethodHandleValueOffset (RuntimeHandleManager* self);
static inline JITINT32 rhm_getRuntimeTypeHandleValueOffset (RuntimeHandleManager* self);
static inline JITINT32 rhm_getRuntimeArgumentHandleValueOffset (RuntimeHandleManager* self);
static inline void rhm_setRuntimeHandleValue (RuntimeHandleManager* self, void* handle, void* value, RuntimeHandles handleType);
static inline void* rhm_getRuntimeHandleValue (RuntimeHandleManager* self, void* handle, RuntimeHandles handleType);
static inline JITINT32 rhm_getRuntimeHandleValueOffset (RuntimeHandleManager* self, RuntimeHandles handleType);
static inline void rhm_loadDataFromAssembly (void);
static inline void internal_lib_runtimehandle_initialize (void);

/**
 * Obtain the ILType of a RuntimeHandle
 *
 * @param self pointer to the RuntimeHandleManager itself
 * @param ilType pointer to the location in memory that will contain the ilType after the execution of this function
 * @param handleType the type of the handle we want to take the ilType of.
 */
static inline TypeDescriptor *internal_fillILRuntimeHandle (RuntimeHandleManager* self, RuntimeHandles handleType);

pthread_once_t runtimeHandleMetadataLock = PTHREAD_ONCE_INIT;

/* Constructor */
void init_runtimeHandleManager (RuntimeHandleManager *self) {
    /* Link methods */
    self->setRuntimeFieldHandleValue = RHM_SETRUNTIMEFIELDHANDLEVALUE;
    self->setRuntimeMethodHandleValue = RHM_SETRUNTIMEMETHODHANDLEVALUE;
    self->setRuntimeTypeHandleValue = RHM_SETRUNTIMETYPEHANDLEVALUE;
    self->setRuntimeArgumentHandleValue = RHM_SETRUNTIMEARGUMENTHANDLEVALUE;
    self->getRuntimeFieldHandleValue = RHM_GETRUNTIMEFIELDHANDLEVALUE;
    self->getRuntimeMethodHandleValue = RHM_GETRUNTIMEMETHODHANDLEVALUE;
    self->getRuntimeTypeHandleValue = RHM_GETRUNTIMETYPEHANDLEVALUE;
    self->getRuntimeArgumentHandleValue = RHM_GETRUNTIMEARGUMENTHANDLEVALUE;
    self->getRuntimeFieldHandleValueOffset = RHM_GETRUNTIMEFIELDHANDLEVALUEOFFSET;
    self->getRuntimeMethodHandleValueOffset = RHM_GETRUNTIMEMETHODHANDLEVALUEOFFSET;
    self->getRuntimeTypeHandleValueOffset = RHM_GETRUNTIMETYPEHANDLEVALUEOFFSET;
    self->getRuntimeArgumentHandleValueOffset = RHM_GETRUNTIMEARGUMENTHANDLEVALUEOFFSET;
    self->fillILRuntimeHandle = RHM_FILLILRUNTIMETYPEHANDLE;
    self->initialize = internal_lib_runtimehandle_initialize;
}

/* Store value inside a System.RuntimeFieldHandle */
static inline void rhm_setRuntimeFieldHandleValue (RuntimeHandleManager* self, void* handle, void* value) {
    rhm_setRuntimeHandleValue(self, handle, value, SYSTEM_RUNTIMEFIELDHANDLE);
}

/* Store value inside a System.RuntimeMethodHandle */
static inline void rhm_setRuntimeMethodHandleValue (RuntimeHandleManager* self, void* handle, void* value) {
    rhm_setRuntimeHandleValue(self, handle, value, SYSTEM_RUNTIMEMETHODHANDLE);
}

/* Store value inside a System.RuntimeTypeHandle */
static inline void rhm_setRuntimeTypeHandleValue (RuntimeHandleManager* self, void* handle, void* value) {
    rhm_setRuntimeHandleValue(self, handle, value, SYSTEM_RUNTIMETYPEHANDLE);
}

/* Store value inside a System.RuntimeArgumentHandle */
static inline void rhm_setRuntimeArgumentHandleValue (RuntimeHandleManager* self, void* handle, void* value) {
    rhm_setRuntimeHandleValue(self, handle, value, SYSTEM_RUNTIMEARGUMENTHANDLE);
}

/* Read handle (a System.RuntimeFieldHandle) value */
static inline void* rhm_getRuntimeFieldHandleValue (RuntimeHandleManager* self, void* handle) {
    return rhm_getRuntimeHandleValue(self, handle, SYSTEM_RUNTIMEFIELDHANDLE);
}

/* Read handle (a System.RuntimeMethodHandle) value */
static inline void* rhm_getRuntimeMethodHandleValue (RuntimeHandleManager* self, void* handle) {
    return rhm_getRuntimeHandleValue(self, handle, SYSTEM_RUNTIMEMETHODHANDLE);
}

/* Read handle (a System.RuntimeTypeHandle) value */
static inline void* rhm_getRuntimeTypeHandleValue (RuntimeHandleManager* self, void* handle) {
    return rhm_getRuntimeHandleValue(self, handle, SYSTEM_RUNTIMETYPEHANDLE);
}

/* Read handle (a System.RuntimeArgumentHandle) value */
static inline void* rhm_getRuntimeArgumentHandleValue (RuntimeHandleManager* self, void* handle) {
    return rhm_getRuntimeHandleValue(self, handle, SYSTEM_RUNTIMEARGUMENTHANDLE);
}

/* Get value field offset inside a System.RuntimeFieldHandle */
static inline JITINT32 rhm_getRuntimeFieldHandleValueOffset (RuntimeHandleManager* self) {
    return rhm_getRuntimeHandleValueOffset(self, SYSTEM_RUNTIMEFIELDHANDLE);
}

/* Get value field offset inside a System.RuntimeMethodHandle */
static inline JITINT32 rhm_getRuntimeMethodHandleValueOffset (RuntimeHandleManager* self) {
    return rhm_getRuntimeHandleValueOffset(self, SYSTEM_RUNTIMEMETHODHANDLE);
}

/* Get value field offset inside a System.RuntimeTypeHandle */
static inline JITINT32 rhm_getRuntimeTypeHandleValueOffset (RuntimeHandleManager* self) {
    return rhm_getRuntimeHandleValueOffset(self, SYSTEM_RUNTIMETYPEHANDLE);
}

/* Get value field offset inside a System.RuntimeArgumentHandle */
static inline JITINT32 rhm_getRuntimeArgumentHandleValueOffset (RuntimeHandleManager* self) {
    return rhm_getRuntimeHandleValueOffset(self, SYSTEM_RUNTIMEARGUMENTHANDLE);
}

/* Write value inside a System.Runtime*Handle */
static inline void rhm_setRuntimeHandleValue (RuntimeHandleManager* self, void* handle,
        void* value, RuntimeHandles handleType) {

    /* Pointer to value field inside given handle */
    void** valueField;

    /* Load offsets from mscorlib assembly, if not yet done */
    PLATFORM_pthread_once(&runtimeHandleMetadataLock, rhm_loadDataFromAssembly);

    /* Store data inside handle */
    valueField = handle + self->valueOffsets[handleType];
    *valueField = value;
}

/* Read value field from a System.Runtime*Handle */
static inline void* rhm_getRuntimeHandleValue (RuntimeHandleManager* self, void* handle,
        RuntimeHandles handleType) {

    /* Pointer to value field inside given handle */
    void** valueField;

    /* Load offsets from mscorlib assembly, if not yet done */
    PLATFORM_pthread_once(&runtimeHandleMetadataLock, rhm_loadDataFromAssembly);

    /* Get pointer to value field */
    valueField = handle + self->valueOffsets[handleType];

    return *valueField;
}

/* Get the offset of the value field inside a System.Runtime*Handle */
static inline JITINT32 rhm_getRuntimeHandleValueOffset (RuntimeHandleManager* self, RuntimeHandles handleType) {

    /* Wanted offset */
    JITINT32 offset;

    /* Load offsets from mscorlib assembly, if not yet done */
    PLATFORM_pthread_once(&runtimeHandleMetadataLock, rhm_loadDataFromAssembly);

    /* Get wanted offset */
    offset = self->valueOffsets[handleType];

    return offset;
}

static inline void internal_lib_runtimehandle_initialize (void) {
    PLATFORM_pthread_once(&runtimeHandleMetadataLock, rhm_loadDataFromAssembly);
}

/* Load data from assembly */
static inline void rhm_loadDataFromAssembly (void) {
    RuntimeHandleManager * self;

    /* System garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Compiler data about the value field */
    FieldDescriptor *valueField;

    /* Needed handles last names */
    char* handleLastNames[RUNTIME_HANDLE_COUNT];

    /* Loop counter */
    JITNUINT i;

    /* Name of the field to load */
    JITINT8 * fieldName;

    /* Cache some pointers, we need they later */
    self = &((cliManager->CLR).runtimeHandleManager);
    garbageCollector = cliManager->gc;

    /* Fill an array with the needed handles unqualified names */
    handleLastNames[0] = "RuntimeFieldHandle";
    handleLastNames[1] = "RuntimeMethodHandle";
    handleLastNames[2] = "RuntimeTypeHandle";
    handleLastNames[3] = "RuntimeArgumentHandle";

    /* Cycle thought all handles */
    for (i = 0; i < RUNTIME_HANDLE_COUNT; i++) {

        /* Load the handle type */
        self->ilTypes[i] = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) handleLastNames[i]);


        /* Look for the value field */
        valueField = self->ilTypes[i]->getFieldFromName(self->ilTypes[i], PNET_VALUEFIELDNAME);

        /* Mono BCL is loaded */
        if (valueField == NULL) {
            switch (i) {
                case SYSTEM_RUNTIMEFIELDHANDLE:
                case SYSTEM_RUNTIMEMETHODHANDLE:
                case SYSTEM_RUNTIMETYPEHANDLE:
                    fieldName = MONO_ALL_VALUEFIELDNAME;
                    break;
                case SYSTEM_RUNTIMEARGUMENTHANDLE:
                    fieldName = MONO_ARGS_VALUEFIELDNAME;
            }
            valueField = self->ilTypes[i]->getFieldFromName(self->ilTypes[i], fieldName);
        }

        /* Retrieve its offset */
        self->valueOffsets[i] = garbageCollector->fetchFieldOffset( valueField);
    }
}

static inline TypeDescriptor *internal_fillILRuntimeHandle (RuntimeHandleManager* self, RuntimeHandles handleType) {

    /* Assertions		*/
    assert(self != NULL);

    /* Load data from assembly if not yet done */
    PLATFORM_pthread_once(&runtimeHandleMetadataLock, rhm_loadDataFromAssembly);

    /* Return				*/
    return self->ilTypes[handleType];
}
