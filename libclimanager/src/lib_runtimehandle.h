/*
 * Copyright (C) 2006  Simone Campanoni, Speziale Ettore
 *
 * iljit - This is a Just-in-time for the CIL language specified with the
 *         ECMA-335
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#ifndef LIB_RUNTIMEHANDLE
#define LIB_RUNTIMEHANDLE

/* Include needed headers */
#include <jitsystem.h>
#include <metadata/metadata_types.h>
#include <platform_API.h>

/**
 * Name of the value field in the PNET BCL
 *
 * This is the name of the field in the PNET BCL that contains the address of
 * compiler handle representation. This field is contained by each of the
 * following value types: System.RuntimeFieldHandle, System.RuntimeMethodHandle,
 * System.RuntimeTypeHandle, System.RuntimeArgumentHandle.
 *
 * This constant is for module internal use only, it is defined here only for
 * documentation purpose, don't use it outside lib_runtimehandle module.
 */
#define PNET_VALUEFIELDNAME (JITINT8 *) "value_"

/**
 * The name of the value field in the MONO BCL
 *
 * This is the name of the field storing a pointer to a compiler data structure
 * inside the System.RuntimeFieldHandle, System.RuntimeMethodHandle and
 * System.RuntimeTypeHandle classes.
 *
 * This constant is for module internal use only, it is defined here only for
 * documentation purpose, don't use it outside lib_runtimehandle module.
 */
#define MONO_ALL_VALUEFIELDNAME (JITINT8 *) "value"

/**
 * The name of the value field in the MONO BCL
 *
 * This is the name of the field storing a pointer to a compiler data structure
 * inside the System.RuntimeArgumentHandle class.
 *
 * This constant is for module internal use only, it is defined here only for
 * documentation purpose, don't use it outside lib_runtimehandle module.
 */
#define MONO_ARGS_VALUEFIELDNAME (JITINT8 *) "args"

/**
 * Number of Runtime*Handle value types managed by this module
 *
 * This constant is for module internal use only, it is defined here only for
 * documentation purpose, don't use it outside lib_runtimehandle module.
 */
#define RUNTIME_HANDLE_COUNT 4

/**
 * Runtime*Handle value types identifiers
 *
 * Define an identifier for each of the followings handle:
 * System.RuntimeFieldHandle, System.RuntimeMethodHandle,
 * System.RuntimeTypeHandle, System.RuntimeArgumentHandle.
 *
 * This enum is for module internal use only, it is defined here only for
 * documentation purpose, don't use it outside lib_runtimehandle module.
 */
typedef enum {
    SYSTEM_RUNTIMEFIELDHANDLE = 0,
    SYSTEM_RUNTIMEMETHODHANDLE = 1,
    SYSTEM_RUNTIMETYPEHANDLE = 2,
    SYSTEM_RUNTIMEARGUMENTHANDLE = 3
} RuntimeHandles;

/**
 * Runtime*Handle manager
 *
 * This module can be used to read/write data from/inside the following value
 * types: RuntimeFieldHandle, RuntimeMethodHandle, RuntimeTypeHandle,
 * RuntimeArgumentHandle. Each of them contains a special field, called value in
 * this description, that stores a pointer to a compiler representation of a
 * field, method, type or argument. This field can be safely read/write with the
 * getter/setter provided by this module. The getter and the setter are thread
 * safe. They aren't typed because inside each can be stored different infos;
 * for example a RuntimeMethodHandle can be used to hold data about a method
 * (CLRMethod in ILDJIT source code) or a constructor (CLRConstructor in ILDJIT
 * source code).
 *
 * @author speziale.ettore@gmail.com
 */
typedef struct RuntimeHandleManager {

    /**
     * Offset of value field, for each managed handle
     *
     * Store the offset of the value field for each managed handle. Don't
     * use this array for reading offsets value, instead use one of the
     * custom getter/setter to read/write a pointer to a compiler resource
     * into an handler.
     */
    JITINT32 valueOffsets[RUNTIME_HANDLE_COUNT];

    /**
     * ILtype of the various runtime handle
     *
     * Contains the ILType of each of the runtime handles that is used
     */
    TypeDescriptor *ilTypes[RUNTIME_HANDLE_COUNT];

    /**
     * Store value into given System.RuntimeFieldHandle
     *
     * @param self like this object
     * @param handle where to write data
     * @param value the data to be written
     */
    void (*setRuntimeFieldHandleValue)(struct RuntimeHandleManager * self, void* handle, void* value);

    /**
     * Store value into given System.RuntimeMethodHandle
     *
     * @param self like this object
     * @param handle where to write data
     * @param value the data to be written
     */
    void (*setRuntimeMethodHandleValue)(struct RuntimeHandleManager * self, void* handle, void* value);

    /**
     * Store value into given System.RuntimeTypeHandle
     *
     * @param self like this object
     * @param handle where to write data
     * @param value the data to be written
     */
    void (*setRuntimeTypeHandleValue)(struct RuntimeHandleManager * self, void* handle, void* value);

    /**
     * Store value into given System.RuntimeArgumentHandle
     *
     * @param self like this object
     * @param handle where to write data
     * @param value the data to be written
     */
    void (*setRuntimeArgumentHandleValue)(struct RuntimeHandleManager * self, void* handle, void* value);

    /**
     * Read value field from given System.RuntimeFieldHandle
     *
     * @param self like this object
     * @param handle where to read data
     *
     * @return a pointer to a field compiler description
     */
    void* (*getRuntimeFieldHandleValue)(struct RuntimeHandleManager * self, void* handle);

    /**
     * Read value field from given System.RuntimeMethodHandle
     *
     * @param self like this object
     * @param handle where to read data
     *
     * @return a pointer to a method compiler description
     */
    void* (*getRuntimeMethodHandleValue)(struct RuntimeHandleManager * self, void* handle);

    /**
     * Read value field from given System.RuntimeTypeHandle
     *
     * @param self like this object
     * @param handle where to read data
     *
     * @return a pointer to a type compiler description
     */
    void* (*getRuntimeTypeHandleValue)(struct RuntimeHandleManager * self, void* handle);

    /**
     * Read value field from given System.RuntimeArgumentHandle
     *
     * @param self like this object
     * @param handle where to read data
     *
     * @return a pointer to an argument compiler description
     */
    void* (*getRuntimeArgumentHandleValue)(struct RuntimeHandleManager * self, void* handle);

    /**
     * Get the offset of the value field into a System.RuntimeFieldHandle
     *
     * @param self like this object
     *
     * @return the offset of the value field
     */
    JITINT32 (*getRuntimeFieldHandleValueOffset)(struct RuntimeHandleManager * self);

    /**
     * Get the offset of the value field into a System.RuntimeMethodHandle
     *
     * @param self like this object
     *
     * @return the offset of the value field
     */
    JITINT32 (*getRuntimeMethodHandleValueOffset)(struct RuntimeHandleManager * self);

    /**
     * Get the offset of the value field into a System.RuntimeTypeHandle
     *
     * @param self like this object
     *
     * @return the offset of the value field
     */
    JITINT32 (*getRuntimeTypeHandleValueOffset)(struct RuntimeHandleManager * self);

    /**
     * Get the offset of the value field into a System.RuntimeArgumentHandle
     *
     * @param self like this object
     *
     * @return the offset of the value field
     */
    JITINT32 (*getRuntimeArgumentHandleValueOffset)(struct RuntimeHandleManager * self);

    /**
     * Obtain the ILType of a RuntimeHandle
     *
     * @param self pointer to the RuntimeHandleManager itself
     * @param ilType pointer to the location in memory that will contain the ilType after the execution of this function
     */
    TypeDescriptor *(*fillILRuntimeHandle)(struct RuntimeHandleManager *self, RuntimeHandles handleType);

    void (*initialize)(void);
} RuntimeHandleManager;

/**
 * Build a new RuntimeHandleManager
 *
 * This functions acts as a constructor for RuntimeHandleManager object in OO
 * language. Remember to call destroyRuntimeHandleManager in order to avoid
 * RuntimeHandleManager related memory leaks.
 *
 * @return a new RuntimeHandleManager object
 */
void init_runtimeHandleManager (RuntimeHandleManager *manager);

#endif /* LIB_RUNTIMEHANDLE */
