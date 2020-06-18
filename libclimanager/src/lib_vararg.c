/*
 * Copyright (C) 2009 - 2010  Campanoni Simone
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
#include <platform_API.h>

// My headers
#include <lib_vararg.h>
#include <climanager_system.h>
#include <small_unicode.h>
// End

extern CLIManager_t *cliManager;

static inline void internal_check_metadata_vararg (void);
static inline void internal_create_arg_iterator (void* object, void* args, JITINT32 pos);
static inline JITINT32 internal_getposnFieldOffset (CLIManager_t *cliManager);
static inline JITINT32 internal_getargsFieldOffset (CLIManager_t *cliManager);
static inline void* internal_getargsField (void *object);
static inline void internal_setargsField (void *object, void*args);
static inline JITINT32 internal_getposnField (void *object);
static inline void internal_setposnField (void *object, JITINT32 posn);
static inline void internal_initialize (void);

pthread_once_t varargMetadataLock = PTHREAD_ONCE_INIT;

void init_varargManager (t_varargManager *varargManager) {

    /* Assertions			*/
    assert(varargManager != NULL);

    varargManager->create_arg_iterator = internal_create_arg_iterator;
    varargManager->getposnField = internal_getposnField;
    varargManager->setposnField = internal_setposnField;
    varargManager->getargsField = internal_getargsField;
    varargManager->setargsField = internal_setargsField;
    varargManager->initialize = internal_initialize;
}

static inline JITINT32 internal_getposnFieldOffset (CLIManager_t *cliManager) {
    /*	get the offsets		*/
    PLATFORM_pthread_once(&varargMetadataLock, internal_check_metadata_vararg);

    /*	return			*/
    return (cliManager->CLR).varargManager.posnFieldOffset;
}

static inline JITINT32 internal_getargsFieldOffset (CLIManager_t *cliManager) {

    /*	get the offsets		*/
    PLATFORM_pthread_once(&varargMetadataLock, internal_check_metadata_vararg);

    /*	return			*/
    return (cliManager->CLR).varargManager.argsFieldOffset;
}

static inline void internal_create_arg_iterator (void* object, void* args, JITINT32 pos) {
    JITINT32 fieldOffsetArgs;
    JITINT32 fieldOffsetPosn;

    /* Initialize the variables			*/
    fieldOffsetArgs = 0;
    fieldOffsetPosn = 0;

    /* Fetch the offsets of the Args and Posn fields	*/
    fieldOffsetArgs = internal_getargsFieldOffset(cliManager);
    fieldOffsetPosn = internal_getposnFieldOffset(cliManager);
    assert(fieldOffsetArgs >= 0);
    assert(fieldOffsetPosn >= 0);

    /* Fetch the offset of the "value_" field of the RuntimeArgumentHandle	*/
    JITINT32 argsValueOffset = (cliManager->CLR).runtimeHandleManager.getRuntimeArgumentHandleValueOffset(&((cliManager->CLR).runtimeHandleManager));
    assert(argsValueOffset >= 0);
    assert((object + fieldOffsetArgs) != NULL);
    assert((args + argsValueOffset) != NULL);
    assert((object + fieldOffsetPosn) != NULL);
    assert(*((void**) (args + argsValueOffset)) != NULL);

    /* Init the address of the arguments list	*/
    *((void**) (object + fieldOffsetArgs)) = *((void**) (args + argsValueOffset));

    /* Init the position into the arguments list	*/
    *((JITINT32*) (object + fieldOffsetPosn)) = pos;

    /* Return					*/
    return;
}

static inline void internal_check_metadata_vararg (void) {

    /* Check if we have to load the metadata	*/
    if ((cliManager->CLR).varargManager.TypedReferenceType == NULL) {


        /* Fetch the System.TypedReference type			*/
        (cliManager->CLR).varargManager.TypedReferenceType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "ArgIterator");

        /* Fetch the type field ID			*/
        (cliManager->CLR).varargManager.argsField = (cliManager->CLR).varargManager.TypedReferenceType->getFieldFromName((cliManager->CLR).varargManager.TypedReferenceType, (JITINT8 *) "cookie1");

        /* Fetch the offset				*/
        (cliManager->CLR).varargManager.argsFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).varargManager.argsField);

        /* NB: cookie2 is reserved to be used together with cookie1 in the future if 64-bit pointers are required	*/

        /* Fetch the value field ID			*/
        (cliManager->CLR).varargManager.posnField = (cliManager->CLR).varargManager.TypedReferenceType->getFieldFromName((cliManager->CLR).varargManager.TypedReferenceType, (JITINT8 *) "cookie3");

        /* Fetch the offset				*/
        (cliManager->CLR).varargManager.posnFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).varargManager.posnField);
    }

    /* Final assertions				*/
    assert((cliManager->CLR).varargManager.argsField != NULL);
    assert((cliManager->CLR).varargManager.posnField != NULL);
    assert((cliManager->CLR).varargManager.TypedReferenceType != NULL);

    /* Return					*/
    return;
}

static inline void internal_setargsField (void *object, void*args) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);

    /* Fetch the length offset					*/
    fieldOffset = internal_getargsFieldOffset(cliManager);

    *((void **) (object + fieldOffset)) = args;
}

static inline void internal_setposnField (void *object, JITINT32 posn) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);

    /* Fetch the length offset					*/
    fieldOffset = internal_getposnFieldOffset(cliManager);

    /* Store the length inside the field of the object		*/
    *((JITINT32 *) (object + fieldOffset)) = posn;

    /* Return							*/
    return;
}

static inline JITINT32 internal_getposnField (void *object) {
    JITINT32 posnFieldOffset;

    /* Initialize the variables		*/
    posnFieldOffset = 0;

    /* Check if the string is null		*/
    if (object == NULL) {
        return 0;
    }

    /* Fetch the length offset	*/
    posnFieldOffset = internal_getposnFieldOffset(cliManager);
    assert(posnFieldOffset >= 0);

    /* Return the lengths		*/
    return *((JITINT32 *) (object + posnFieldOffset));
}

static inline void * internal_getargsField (void *object) {
    JITINT32 fieldOffset;

    /* Assertions					*/
    assert(object != NULL);

    /* Fetch the length offset	*/
    fieldOffset = internal_getargsFieldOffset(cliManager);
    assert(fieldOffset >= 0);

    return *((void **) (object + fieldOffset));
}

static inline void internal_initialize (void) {
    PLATFORM_pthread_once(&varargMetadataLock, internal_check_metadata_vararg);
}
