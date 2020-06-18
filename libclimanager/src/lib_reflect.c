/*
 * Copyright (C) 2006  Campanoni Simone
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compiler_memory_manager.h>
#include <jit_metadata.h>
#include <base_symbol.h>
#include <platform_API.h>

// My headers
#include <lib_reflect.h>
#include <climanager_system.h>
// End

extern CLIManager_t *cliManager;

static inline void internal_destroyReflectManager (t_reflectManager *self);
static inline void internal_libreflect_check_metadata (void);
static inline void * internal_getType (TypeDescriptor *ilType);
static inline void * internal_getAssembly (BasicAssembly *ilAssembly);
static inline void * internal_getProperty (PropertyDescriptor *cilProperty);
static inline void * internal_getMethod (MethodDescriptor *cilMethod);
static inline void * internal_getConstructor (MethodDescriptor *cilConstructor);
static inline void * internal_getField (FieldDescriptor *cilField);
static inline void * internal_getEvent (EventDescriptor *cilEvent);
static inline void * internal_getParameter (ParamDescriptor *cilParameter);
static inline void * internal_getModule (BasicModule *cilModule);
static inline CLRType * internal_getCLRType (TypeDescriptor *class);
static inline ir_symbol_t * internal_getCLRTypeSymbol (TypeDescriptor *class);
static inline void internal_typeSetPrivateData (void *object, CLRType *class);
static inline void internal_assemblySetPrivateData (void *object, CLRAssembly *clrAssembly);
static inline void internal_propertySetPrivateData (void *object, CLRProperty *clrProperty);
static inline void internal_methodSetPrivateData (void *object, CLRMethod *clrMethod);
static inline void internal_constructorSetPrivateData (void *object, CLRConstructor *clrConstructor);
static inline void internal_fieldSetPrivateData (void *object, CLRField *clrField);
static inline void internal_eventSetPrivateData (void *object, CLREvent *clrEvent);
static inline void internal_parameterSetPrivateData (void *object, CLRParameter *clrParameter);
static inline void internal_moduleSetPrivateData (void *object, CLRModule *clrModule);
static inline CLRType * internal_getPrivateDataFromType (void *object);
static inline CLRAssembly * internal_getPrivateDataFromAssembly (void *object);
static inline CLRProperty * internal_getPrivateDataFromProperty (void *object);
static inline CLRMethod * internal_getPrivateDataFromMethod (void *object);
static inline CLRConstructor * internal_getPrivateDataFromConstructor (void *object);
static inline CLRField * internal_getPrivateDataFromField (void *object);
static inline CLREvent * internal_getPrivateDataFromEvent (void *object);
static inline CLRParameter * internal_getPrivateDataFromParameter (void *object);
static inline CLRModule * internal_getPrivateDataFromModule (void *object);
static inline void internal_lib_reflect_initialize (void);
static inline ir_value_t clrTypeResolve (ir_symbol_t *symbol);
static inline void clrTypeSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void clrTypeDump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t * internal_getCLRTypeSymbol (TypeDescriptor *ID);
static inline ir_symbol_t * clrTypeDeserialize (void *mem, JITUINT32 memBytes);

/* Metadata lock */
pthread_once_t rm_metadataLock = PTHREAD_ONCE_INIT;

void init_reflectManager (t_reflectManager *reflectManager) {

    /* Initialize the functions			*/
    reflectManager->getType = internal_getType;
    reflectManager->getAssembly = internal_getAssembly;
    reflectManager->getProperty = internal_getProperty;
    reflectManager->getMethod = internal_getMethod;
    reflectManager->getConstructor = internal_getConstructor;
    reflectManager->getField = internal_getField;
    reflectManager->getEvent = internal_getEvent;
    reflectManager->getParameter = internal_getParameter;
    reflectManager->getModule = internal_getModule;
    reflectManager->getCLRTypeSymbol = internal_getCLRTypeSymbol;
    reflectManager->getPrivateDataFromType = internal_getPrivateDataFromType;
    reflectManager->getPrivateDataFromAssembly = internal_getPrivateDataFromAssembly;
    reflectManager->getPrivateDataFromProperty = internal_getPrivateDataFromProperty;
    reflectManager->getPrivateDataFromMethod = internal_getPrivateDataFromMethod;
    reflectManager->getPrivateDataFromConstructor = internal_getPrivateDataFromConstructor;
    reflectManager->getPrivateDataFromField = internal_getPrivateDataFromField;
    reflectManager->getPrivateDataFromEvent = internal_getPrivateDataFromEvent;
    reflectManager->getPrivateDataFromParameter = internal_getPrivateDataFromParameter;
    reflectManager->getPrivateDataFromModule = internal_getPrivateDataFromModule;
    reflectManager->initialize = internal_lib_reflect_initialize;
    reflectManager->destroy = internal_destroyReflectManager;

    reflectManager->clrAssemblies	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrModule	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrTypes	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrProperties	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrMethods	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrConstructors	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrFields	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrEvents	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    reflectManager->clrParameter	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(reflectManager->clrAssemblies	!= NULL);
    assert(reflectManager->clrModule	!= NULL);
    assert(reflectManager->clrTypes		!= NULL);
    assert(reflectManager->clrProperties	!= NULL);
    assert(reflectManager->clrMethods	!= NULL);
    assert(reflectManager->clrConstructors	!= NULL);
    assert(reflectManager->clrFields	!= NULL);
    assert(reflectManager->clrEvents	!= NULL);
    assert(reflectManager->clrParameter	!= NULL);

    IRSYMBOL_registerSymbolManager(CLR_TYPE_SYMBOL, clrTypeResolve, clrTypeSerialize, clrTypeDump, clrTypeDeserialize);

    return ;
}

CLRType * CLIMANAGER_getCLRType (TypeDescriptor *type) {
    return internal_getCLRType(type);
}

static inline void * internal_getModule (BasicModule *cilModule) {
    CLRModule       *clrModule;

#ifdef DEBUG
    JITINT8         *name;
#endif

    /* Assertions			*/
    assert(cilModule != NULL);
    PDEBUG("LIB_REFLECT: getModule: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getModule:         Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    clrModule = NULL;

    /* Fetch the instance of System.Reflection.Module of    *
     * the binary						*/
#ifdef DEBUG
    t_binary_information *binary = (t_binary_information *) cilModule->binary;
    name = get_string(&((binary->metadata).streams_metadata.string_stream), ((BasicModule *) cilModule)->name);
    assert(name != NULL);
    PDEBUG("LIB_REFLECT: getModule:         Check if the instance of %s already exist\n", name);
#endif

    xanHashTable_lock((cliManager->CLR).reflectManager.clrModule);
    clrModule = xanHashTable_lookup((cliManager->CLR).reflectManager.clrModule, cilModule);
    if (clrModule == NULL) {

        /* We have to make a new object instance of System.Reflection.Module	*
         * bound to the caller binary						*/
#ifdef DEBUG
        PDEBUG("LIB_REFLECT: getModule:                 The event %s is not made yet\n", name);
#endif
        clrModule = allocMemory(sizeof(CLRModule));
        assert(clrModule != NULL);
        clrModule->ID = cilModule;
        clrModule->type = CLRMODULE;

        /* Call the GC to allocate a new instance                               */
#ifdef DEBUG
        PDEBUG("LIB_REFLECT: getModule:                 Create the instance of the property %s\n", name);
#endif
        clrModule->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrModule, 0);
        if (clrModule->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(clrModule->object != NULL);

        /* Store inside the field privateData the property			*/
        PDEBUG("LIB_REFLECT: getModule:                 Store the field\n");
        internal_moduleSetPrivateData(clrModule->object, clrModule);

        /* Insert the new System.Reflection.Module	*
         * object inside the classes HASH table		*/
#ifdef DEBUG
        PDEBUG("LIB_REFLECT: getModule:                 Insert the property %s inside the hash table\n", name);
#endif

        xanHashTable_insert((cliManager->CLR).reflectManager.clrModule, cilModule, clrModule);
    }
    PDEBUG("LIB_REFLECT: getModule:         Module  = %p\n", clrModule);
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrModule);
    assert(clrModule != NULL);
    assert(clrModule->ID != NULL);
    assert(clrModule->object != NULL);

    /* Return the instance of System.Reflection.Module	*/
    PDEBUG("LIB_REFLECT: getModule: Exit\n");
    return clrModule->object;
}

static inline void * internal_getParameter (ParamDescriptor *cilParameter) {
    CLRParameter	*class;

    /* Assertions			*/
    assert(cilParameter != NULL);
    PDEBUG("LIB_REFLECT: getParameter: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getParameter:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getParameter:       Check if the instance of %s already exist\n", cilParameter->getName(cilParameter));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrParameter);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrParameter, cilParameter);
    if (class == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getParameter:               The Type %s is not made yet\n", cilParameter->getName(cilParameter));
        class = allocMemory(sizeof(CLRParameter));
        assert(class != NULL);
        class->ID = cilParameter;
        class->type = CLRPARAMETER;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getParameter:               Create the instance of the assembly %s\n", cilParameter->getName(cilParameter));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrParameter, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getParameter:               Store the field\n");
        internal_parameterSetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getParameter:               Insert the assembly %s inside the hash table\n", cilParameter->getName(cilParameter));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrParameter, cilParameter, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrParameter);
    PDEBUG("LIB_REFLECT: getParameter:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getParameter: Exit\n");
    return class->object;
}

static inline void * internal_getEvent (EventDescriptor *cilEvent) {
    CLREvent	*class;

    /* Assertions			*/
    assert(cilEvent != NULL);
    PDEBUG("LIB_REFLECT: getEvent: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getEvent:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getEvent:       Check if the instance of %s already exist\n", cilEvent->getName(cilEvent));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrEvents);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrEvents, cilEvent);
    if (class == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getEvent:               The Type %s is not made yet\n", cilEvent->getName(cilEvent));
        class = allocMemory(sizeof(CLREvent));
        assert(class != NULL);
        class->ID = cilEvent;
        class->type = CLREVENT;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getEvent:               Create the instance of the assembly %s\n", cilEvent->getName(cilEvent));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrEvent, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getEvent:               Store the field\n");
        internal_eventSetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getEvent:               Insert the assembly %s inside the hash table\n", cilEvent->getName(cilEvent));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrEvents, cilEvent, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrEvents);
    PDEBUG("LIB_REFLECT: getEvent:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getEvent: Exit\n");
    return class->object;
}

static inline void * internal_getField (FieldDescriptor *cilField) {
    CLRField	*class;

    /* Assertions			*/
    assert(cilField != NULL);
    PDEBUG("LIB_REFLECT: getField: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getField:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getField:       Check if the instance of %s already exist\n", cilField->getName(cilField));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrFields);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrFields, cilField);
    if (class == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getField:               The Type %s is not made yet\n", cilField->getName(cilField));
        class = allocMemory(sizeof(CLRField));
        assert(class != NULL);
        class->ID = cilField;
        class->type = CLRFIELD;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getField:               Create the instance of the assembly %s\n", cilField->getName(cilField));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrField, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getField:               Store the field\n");
        internal_fieldSetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getField:               Insert the assembly %s inside the hash table\n", cilField->getName(cilField));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrFields, cilField, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrFields);
    PDEBUG("LIB_REFLECT: getField:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getField: Exit\n");
    return class->object;
}

static inline void * internal_getConstructor (MethodDescriptor *cilConstructor) {
    CLRConstructor	*class;

    /* Assertions			*/
    assert(cilConstructor != NULL);
    PDEBUG("LIB_REFLECT: getConstructor: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getConstructor:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getConstructor:       Check if the instance of %s already exist\n", cilConstructor->getName(cilConstructor));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrConstructors);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrConstructors, cilConstructor);
    if (class == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getConstructor:               The Type %s is not made yet\n", cilConstructor->getName(cilConstructor));
        class = allocMemory(sizeof(CLRConstructor));
        assert(class != NULL);
        class->ID = cilConstructor;
        class->type = CLRCONSTRUCTOR;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getConstructor:               Create the instance of the assembly %s\n", cilConstructor->getName(cilConstructor));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrConstructor, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getConstructor:               Store the field\n");
        internal_constructorSetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getConstructor:               Insert the assembly %s inside the hash table\n", cilConstructor->getName(cilConstructor));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrConstructors, cilConstructor, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrConstructors);
    PDEBUG("LIB_REFLECT: getConstructor:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getConstructor: Exit\n");
    return class->object;
}

static inline void * internal_getMethod (MethodDescriptor *cilMethod) {
    CLRMethod	*class;

    /* Assertions			*/
    assert(cilMethod != NULL);
    PDEBUG("LIB_REFLECT: getMethod: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getMethod:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getMethod:       Check if the instance of %s already exist\n", cilMethod->getName(cilMethod));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrMethods);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrMethods, cilMethod);
    if (class == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getMethod:               The Type %s is not made yet\n", cilMethod->getName(cilMethod));
        class = allocMemory(sizeof(CLRMethod));
        assert(class != NULL);
        class->ID = cilMethod;
        class->type = CLRMETHOD;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getMethod:               Create the instance of the assembly %s\n", cilMethod->getName(cilMethod));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrMethod, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getMethod:               Store the field\n");
        internal_methodSetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getMethod:               Insert the assembly %s inside the hash table\n", cilMethod->getName(cilMethod));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrMethods, cilMethod, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrMethods);
    PDEBUG("LIB_REFLECT: getMethod:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getMethod: Exit\n");
    return class->object;
}

static inline void * internal_getProperty (PropertyDescriptor *cilProperty) {
    CLRProperty	*class;

    /* Assertions			*/
    assert(cilProperty != NULL);
    PDEBUG("LIB_REFLECT: getProperty: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getProperty:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getProperty:       Check if the instance of %s already exist\n", cilProperty->getName(cilProperty));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrProperties);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrProperties, cilProperty);
    if (class == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getProperty:               The Type %s is not made yet\n", cilProperty->getName(cilProperty));
        class = allocMemory(sizeof(CLRProperty));
        assert(class != NULL);
        class->ID = cilProperty;
        class->type = CLRPROPERTY;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getProperty:               Create the instance of the assembly %s\n", cilProperty->getName(cilProperty));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrProperty, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getProperty:               Store the field\n");
        internal_propertySetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getProperty:               Insert the assembly %s inside the hash table\n", cilProperty->getName(cilProperty));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrProperties, cilProperty, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrProperties);
    PDEBUG("LIB_REFLECT: getProperty:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getProperty: Exit\n");
    return class->object;
}

static inline void * internal_getAssembly (BasicAssembly *ilAssembly) {
    CLRAssembly     *clrAssembly;

#ifdef DEBUG
    JITINT8         *name;
#endif

    /* Assertions			*/
    assert(ilAssembly != NULL);
    PDEBUG("LIB_REFLECT: getAssembly: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getAssembly:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    clrAssembly = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
#ifdef DEBUG
    t_binary_information *binary = (t_binary_information *) ilAssembly->binary;
    name = get_string(&((binary->metadata).streams_metadata.string_stream), ilAssembly->name);
    assert(name != NULL);

    PDEBUG("LIB_REFLECT: getAssembly:       Check if the instance of %s already exist\n", name);
#endif
    xanHashTable_lock((cliManager->CLR).reflectManager.clrAssemblies);
    clrAssembly = xanHashTable_lookup((cliManager->CLR).reflectManager.clrAssemblies, ilAssembly);
    if (clrAssembly == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
#ifdef DEBUG
        PDEBUG("LIB_REFLECT: getAssembly:               The assembly %s is not made yet\n", name);
#endif
        clrAssembly = allocMemory(sizeof(CLRAssembly));
        assert(clrAssembly != NULL);
        clrAssembly->ID = ilAssembly;
        clrAssembly->type = CLRASSEMBLY;

        /* Call the GC to allocate a new instance                               */
#ifdef DEBUG
        PDEBUG("LIB_REFLECT: getAssembly:               Create the instance of the assembly %s\n", name);
#endif
        clrAssembly->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrAssembly, 0);
        if (clrAssembly->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        }
        assert(clrAssembly->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getAssembly:               Store the field\n");
        internal_assemblySetPrivateData(clrAssembly->object, clrAssembly);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
#ifdef DEBUG
        PDEBUG("LIB_REFLECT: getAssembly:               Insert the assembly %s inside the hash table\n", name);
#endif
        xanHashTable_insert((cliManager->CLR).reflectManager.clrAssemblies, ilAssembly, clrAssembly);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrAssemblies);
    PDEBUG("LIB_REFLECT: getAssembly:       Assembly        = %p\n", clrAssembly);
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);
    assert(clrAssembly->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getAssembly: Exit\n");
    return clrAssembly->object;
}

static inline void * internal_getType (TypeDescriptor *ilType) {
    CLRType	*class;

    /* Assertions			*/
    assert(ilType != NULL);
    PDEBUG("LIB_REFLECT: getType: Start\n");

    /* Check the metadata				*/
    PDEBUG("LIB_REFLECT: getType:       Check the metadata\n");
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getType:       Check if the instance of %s already exist\n", ilType->getName(ilType));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrTypes);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrTypes, ilType);
    if (class == NULL) {

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getType:               The Type %s is not made yet\n", ilType->getName(ilType));
        class = allocMemory(sizeof(CLRType));
        assert(class != NULL);
        class->ID = ilType;
        class->type = CLRTYPE;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getType:               Create the instance of the assembly %s\n", ilType->getName(ilType));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrType, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
            abort();
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getType:               Store the field\n");
        internal_typeSetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getType:               Insert the assembly %s inside the hash table\n", ilType->getName(ilType));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrTypes, ilType, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrTypes);
    PDEBUG("LIB_REFLECT: getType:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);
    assert((*(((cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrType))->virtualTable)) != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getType: Exit\n");
    return class->object;
}

static inline void internal_libreflect_check_metadata (void) {

    /* Fetch the System.Reflection.ClrType type	*/
    (cliManager->CLR).reflectManager.ClrType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "ClrType");
    assert((cliManager->CLR).reflectManager.ClrType != NULL);
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrType);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.typePrivateDataField = (cliManager->CLR).reflectManager.ClrType->getFieldFromName((cliManager->CLR).reflectManager.ClrType, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.typePrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.typePrivateDataField);

    /* Fetch the System.Reflection.Assembly type	*/
    (cliManager->CLR).reflectManager.ClrAssembly = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "Assembly");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrAssembly);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.assemblyPrivateDataField = (cliManager->CLR).reflectManager.ClrAssembly->getFieldFromName((cliManager->CLR).reflectManager.ClrAssembly, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.assemblyPrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.assemblyPrivateDataField);

    /* Fetch the System.Reflection.ClrProperty type	*/
    (cliManager->CLR).reflectManager.ClrProperty = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "ClrProperty");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrProperty);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.propertyPrivateDataField = (cliManager->CLR).reflectManager.ClrProperty->getFieldFromName((cliManager->CLR).reflectManager.ClrProperty, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.propertyPrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.propertyPrivateDataField);

    /* Fetch the System.Reflection.ClrMethod type	*/
    (cliManager->CLR).reflectManager.ClrMethod = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "ClrMethod");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrMethod);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.methodPrivateDataField = (cliManager->CLR).reflectManager.ClrMethod->getFieldFromName((cliManager->CLR).reflectManager.ClrMethod, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.methodPrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.methodPrivateDataField);

    /* Fetch the System.Reflection.ClrConstructor type	*/
    (cliManager->CLR).reflectManager.ClrConstructor = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "ClrConstructor");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrConstructor);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.constructorPrivateDataField = (cliManager->CLR).reflectManager.ClrConstructor->getFieldFromName((cliManager->CLR).reflectManager.ClrConstructor, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.constructorPrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.constructorPrivateDataField);

    /* Fetch the System.Reflection.ClrField type	*/
    (cliManager->CLR).reflectManager.ClrField = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "ClrField");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrField);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.fieldPrivateDataField = (cliManager->CLR).reflectManager.ClrField->getFieldFromName((cliManager->CLR).reflectManager.ClrField, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.fieldPrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.fieldPrivateDataField);

    /* Fetch the System.Reflection.ClrEvent type	*/
    (cliManager->CLR).reflectManager.ClrEvent = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "ClrEvent");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrEvent);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.eventPrivateDataField = (cliManager->CLR).reflectManager.ClrEvent->getFieldFromName((cliManager->CLR).reflectManager.ClrEvent, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.eventPrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.eventPrivateDataField);

    /* Fetch the System.Reflection.ClrParameter type	*/
    (cliManager->CLR).reflectManager.ClrParameter = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "ClrParameter");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrParameter);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.parameterPrivateDataField = (cliManager->CLR).reflectManager.ClrParameter->getFieldFromName((cliManager->CLR).reflectManager.ClrParameter, (JITINT8 *) "privateData");

    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.parameterPrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.parameterPrivateDataField);

    /* Fetch the System.Reflection.ClrModule type	*/
    (cliManager->CLR).reflectManager.ClrModule = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Reflection", (JITINT8 *) "Module");
    (cliManager->layoutManager).layoutType(&(cliManager->layoutManager), (cliManager->CLR).reflectManager.ClrModule);

    /* Fetch the privateData field ID		*/
    (cliManager->CLR).reflectManager.modulePrivateDataField = (cliManager->CLR).reflectManager.ClrModule->getFieldFromName((cliManager->CLR).reflectManager.ClrModule, (JITINT8 *) "privateData");


    /* Fetch the offset				*/
    (cliManager->CLR).reflectManager.modulePrivateDataFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).reflectManager.modulePrivateDataField);

    /* Final assertions				*/
    assert((cliManager->CLR).reflectManager.typePrivateDataField != NULL);
    assert((cliManager->CLR).reflectManager.assemblyPrivateDataField != NULL);
    assert((cliManager->CLR).reflectManager.ClrType != NULL);
    assert((cliManager->CLR).reflectManager.ClrAssembly != NULL);
    assert((cliManager->CLR).reflectManager.ClrProperty != NULL);
    assert((cliManager->CLR).reflectManager.ClrMethod != NULL);
    assert((cliManager->CLR).reflectManager.ClrConstructor != NULL);
    assert((cliManager->CLR).reflectManager.ClrField != NULL);
    assert((cliManager->CLR).reflectManager.ClrEvent != NULL);
    assert((cliManager->CLR).reflectManager.ClrParameter != NULL);
    assert((cliManager->CLR).reflectManager.ClrModule != NULL);

    /* Return					*/
    return;
}

static inline CLRModule * internal_getPrivateDataFromModule (void *object) {
    JITINT32 fieldOffset;
    CLRModule       *clrModule;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.modulePrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrModule = (*((CLRModule **) (object + fieldOffset)));
    assert(clrModule != NULL);
    assert(clrModule->ID != NULL);
    assert(clrModule->object != NULL);

    /* Return							*/
    return clrModule;
}
static inline CLRParameter * internal_getPrivateDataFromParameter (void *object) {
    JITINT32 fieldOffset;
    CLRParameter    *clrParameter;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.parameterPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrParameter = (*((CLRParameter **) (object + fieldOffset)));
    assert(clrParameter != NULL);
    assert(clrParameter->ID != NULL);
    assert(clrParameter->object != NULL);

    /* Return							*/
    return clrParameter;
}

static inline CLREvent * internal_getPrivateDataFromEvent (void *object) {
    JITINT32 fieldOffset;
    CLREvent        *clrEvent;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.eventPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrEvent = (*((CLREvent **) (object + fieldOffset)));
    assert(clrEvent != NULL);
    assert(clrEvent->ID != NULL);
    assert(clrEvent->object != NULL);

    /* Return							*/
    return clrEvent;
}

static inline CLRField * internal_getPrivateDataFromField (void *object) {
    JITINT32 fieldOffset;
    CLRField        *clrField;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.fieldPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrField = (*((CLRField **) (object + fieldOffset)));
    assert(clrField != NULL);
    assert(clrField->ID != NULL);
    assert(clrField->object != NULL);

    /* Return							*/
    return clrField;
}

static inline CLRConstructor * internal_getPrivateDataFromConstructor (void *object) {
    JITINT32 fieldOffset;
    CLRConstructor  *clrConstructor;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.constructorPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrConstructor = (*((CLRConstructor **) (object + fieldOffset)));
    assert(clrConstructor != NULL);
    assert(clrConstructor->ID != NULL);
    assert(clrConstructor->object != NULL);

    /* Return							*/
    return clrConstructor;
}

static inline CLRMethod * internal_getPrivateDataFromMethod (void *object) {
    JITINT32 fieldOffset;
    CLRMethod       *clrMethod;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.methodPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrMethod = (*((CLRMethod **) (object + fieldOffset)));
    assert(clrMethod != NULL);
    assert(clrMethod->ID != NULL);
    assert(clrMethod->object != NULL);

    /* Return							*/
    return clrMethod;
}

static inline CLRProperty * internal_getPrivateDataFromProperty (void *object) {
    JITINT32 fieldOffset;
    CLRProperty     *clrProperty;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.propertyPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrProperty = (*((CLRProperty **) (object + fieldOffset)));
    assert(clrProperty != NULL);
    assert(clrProperty->ID != NULL);
    assert(clrProperty->object != NULL);

    /* Return							*/
    return clrProperty;
}

static inline CLRAssembly * internal_getPrivateDataFromAssembly (void *object) {
    JITINT32 fieldOffset;
    CLRAssembly     *clrAssembly;

    /* Assertions							*/
    assert(object != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.assemblyPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    clrAssembly = (*((CLRAssembly **) (object + fieldOffset)));
    assert(clrAssembly != NULL);
    assert(clrAssembly->ID != NULL);
    assert(clrAssembly->object != NULL);

    /* Return							*/
    return clrAssembly;
}

static inline CLRType * internal_getPrivateDataFromType (void *object) {
    JITINT32 fieldOffset;
    CLRType         *class;

    /* Assertions							*/
    assert(object != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.typePrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    class = (*((CLRType **) (object + fieldOffset)));
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return							*/
    return class;
}

static inline void internal_moduleSetPrivateData (void *object, CLRModule *class) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(class != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.modulePrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &class, sizeof(CLRModule *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromModule(object) == class);

    /* Return							*/
    return;
}
static inline void internal_parameterSetPrivateData (void *object, CLRParameter *class) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(class != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.parameterPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &class, sizeof(CLRParameter *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromParameter(object) == class);

    /* Return							*/
    return;
}


static inline void internal_typeSetPrivateData (void *object, CLRType *class) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(class != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.typePrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &class, sizeof(CLRType *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromType(object) == class);

    /* Return							*/
    return;
}

static inline void internal_eventSetPrivateData (void *object, CLREvent *clrEvent) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(clrEvent != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.eventPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &clrEvent, sizeof(CLREvent *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromEvent(object) == clrEvent);

    /* Return							*/
    return;
}

static inline void internal_fieldSetPrivateData (void *object, CLRField *clrField) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(clrField != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.fieldPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &clrField, sizeof(CLRField *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromField(object) == clrField);

    /* Return							*/
    return;
}

static inline void internal_constructorSetPrivateData (void *object, CLRConstructor *clrConstructor) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(clrConstructor != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.constructorPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &clrConstructor, sizeof(CLRConstructor *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromConstructor(object) == clrConstructor);

    /* Return							*/
    return;
}

static inline void internal_methodSetPrivateData (void *object, CLRMethod *clrMethod) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(clrMethod != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.methodPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &clrMethod, sizeof(CLRMethod *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromMethod(object) == clrMethod);

    /* Return							*/
    return;
}

static inline void internal_propertySetPrivateData (void *object, CLRProperty *clrProperty) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(clrProperty != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.propertyPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &clrProperty, sizeof(CLRProperty *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromProperty(object) == clrProperty);

    /* Return							*/
    return;
}

static inline void internal_assemblySetPrivateData (void *object, CLRAssembly *clrAssembly) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);
    assert(clrAssembly != NULL);
    assert((cliManager->entryPoint).binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).reflectManager.assemblyPrivateDataFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &clrAssembly, sizeof(CLRAssembly *));
    assert((cliManager->CLR).reflectManager.getPrivateDataFromAssembly(object) == clrAssembly);

    /* Return							*/
    return;
}

static inline void internal_lib_reflect_initialize (void) {

    /* Initialize the module	*/
    PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

    /* Return			*/
    return;
}

static inline ir_value_t clrTypeResolve (ir_symbol_t *symbol) {
    ir_value_t value;
    TypeDescriptor *type;

    type = (TypeDescriptor *) symbol->data;
    assert(type != NULL);

    value.v = (JITNUINT) internal_getCLRType(type);
    return value;
}

static inline void clrTypeSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    JITUINT32	bytesWritten;

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= allocFunction(*memBytesAllocated);

    /* Fetch the type.
     */
    TypeDescriptor *type = (TypeDescriptor *) symbol->data;
    assert(type != NULL);
    ir_symbol_t *typeSymbol = (cliManager->translator).getTypeDescriptorSymbol(cliManager, type);

    /* Serialize the symbol ID.
     */
    bytesWritten		= ILDJIT_writeIntegerValueInMemory(*mem, 0, *memBytesAllocated, typeSymbol->id, JITFALSE);
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void clrTypeDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    TypeDescriptor *type;

    type = (TypeDescriptor *) symbol->data;
    assert(type != NULL);

    fprintf(fileToWrite, "Pointer to CLRType of %s", type->getCompleteName(type));
}

static inline ir_symbol_t * internal_getCLRTypeSymbol (TypeDescriptor *ID) {
    ir_symbol_t	*s;
    s	= IRSYMBOL_createSymbol(CLR_TYPE_SYMBOL, (void *) ID);
    assert(s != NULL);
    return s;
}

static inline ir_symbol_t * clrTypeDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32	typeSymbolID;
    ir_symbol_t	*s;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Deserialize the symbol ID.
     */
    ILDJIT_readIntegerValueFromMemory(mem, 0, &typeSymbolID);

    /* Fetch the symbol.
     */
    ir_symbol_t *symbol = IRSYMBOL_loadSymbol(typeSymbolID);
    TypeDescriptor *type = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(symbol).v);

    s	= internal_getCLRTypeSymbol(type);
    assert(s != NULL);

    return s;
}

static inline CLRType * internal_getCLRType (TypeDescriptor *ID) {
    CLRType	*class;

    /* Assertions			*/
    assert(ID != NULL);
    PDEBUG("LIB_REFLECT: getCLRType: Start\n");

    /* Initialize the variables	*/
    class = NULL;

    /* Fetch the instance of System.Reflection.Assembly of  *
     * the binary						*/
    PDEBUG("LIB_REFLECT: getCLRType:       Check if the instance of %s already exist\n", ID->getName(ID));
    xanHashTable_lock((cliManager->CLR).reflectManager.clrTypes);
    class = xanHashTable_lookup((cliManager->CLR).reflectManager.clrTypes, ID);
    if (class == NULL) {

        /* Check the metadata				*/
        PDEBUG("LIB_REFLECT: getCLRType:       Check the metadata\n");
        PLATFORM_pthread_once(&rm_metadataLock, internal_libreflect_check_metadata);

        /* We have to make a new object instance of System.Reflection.Assembly	*
         * bound to the caller binary						*/
        PDEBUG("LIB_REFLECT: getCLRType:               The CLRType %s is not made yet\n", ID->getName(ID));
        class = allocMemory(sizeof(CLRType));
        assert(class != NULL);
        class->ID = ID;
        class->type = CLRTYPE;

        /* Call the GC to allocate a new instance                               */
        PDEBUG("LIB_REFLECT: getCLRType:               Create the instance of the assembly %s\n", ID->getName(ID));
        class->object = cliManager->gc->allocPermanentObject((cliManager->CLR).reflectManager.ClrType, 0);
        if (class->object == NULL) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
            abort();
        }
        assert(class->object != NULL);

        /* Store inside the field privateData the assembly			*/
        PDEBUG("LIB_REFLECT: getCLRType:               Store the field\n");
        internal_typeSetPrivateData(class->object, class);

        /* Insert the new System.Reflection.Assembly	*
         * object inside the classes HASH table		*/
        PDEBUG("LIB_REFLECT: getAssembly:               Insert the assembly %s inside the hash table\n", ID->getName(ID));
        xanHashTable_insert((cliManager->CLR).reflectManager.clrTypes, ID, class);
    }
    xanHashTable_unlock((cliManager->CLR).reflectManager.clrTypes);
    PDEBUG("LIB_REFLECT: getAssembly:       Assembly        = %p\n", class);
    assert(class != NULL);
    assert(class->ID != NULL);
    assert(class->object != NULL);

    /* Return the instance of System.Reflection.Assembly	*/
    PDEBUG("LIB_REFLECT: getAssembly: Exit\n");
    return class;
}

static inline void internal_destroyReflectManager (t_reflectManager *self) {
    XanHashTableItem	*item;

    /* Assertions.
     */
    assert(self != NULL);

    /* Destroy the context of tables.
     */
    item	= xanHashTable_first(self->clrTypes);
    while (item != NULL) {
        CLRType	*class;
        class	= item->element;
        freeFunction(class->object - HEADER_FIXED_SIZE);
        freeFunction(class);
        item	= xanHashTable_next(self->clrTypes, item);
    }

    /* Destroy the hash tables.
     */
    xanHashTable_destroyTable(self->clrTypes);
    xanHashTable_destroyTable(self->clrAssemblies);
    xanHashTable_destroyTable(self->clrModule);
    xanHashTable_destroyTable(self->clrProperties);
    xanHashTable_destroyTable(self->clrMethods);
    xanHashTable_destroyTable(self->clrConstructors);
    xanHashTable_destroyTable(self->clrFields);
    xanHashTable_destroyTable(self->clrEvents);
    xanHashTable_destroyTable(self->clrParameter);
}

//#undef PDEBUG
//#define PDEBUG(fmt, args ...)
