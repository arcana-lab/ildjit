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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <platform_API.h>

// My headers
#include <lib_typedReference.h>
#include <small_unicode.h>
#include <climanager_system.h>
// End

extern CLIManager_t *cliManager;

static inline void internal_check_metadata_typedref (void);
static inline JITINT32 internal_getValueOffset (t_typedReferenceManager *self);
static inline JITINT32 internal_getTypeOffset (t_typedReferenceManager *self);
static inline TypeDescriptor *internal_fillILTypedRefType ();
static inline TypeDescriptor *internal_fillILTypedRefArray ();
static inline void *internal_getValue (void *thisTypedRef);
static inline void internal_create_typed_ref (void* object, void* type, void* value);
static inline void *internal_getType (void *thisTypedRef);
static inline void internal_typedReference_initialize (void);
static inline void internal_copy (void *dest, void *src);

/* Metadata lock */
pthread_once_t typedRefMetadataLock = PTHREAD_ONCE_INIT;

void init_typedReferenceManager (t_typedReferenceManager *typedReferenceManager) {

    typedReferenceManager->fillILTypedRefType = internal_fillILTypedRefType;
    typedReferenceManager->fillILTypedRefArray = internal_fillILTypedRefArray;
    typedReferenceManager->create_typed_ref = internal_create_typed_ref;
    typedReferenceManager->getValue = internal_getValue;
    typedReferenceManager->getType = internal_getType;
    typedReferenceManager->getValueOffset = internal_getValueOffset;
    typedReferenceManager->getTypeOffset = internal_getTypeOffset;
    typedReferenceManager->initialize = internal_typedReference_initialize;
    typedReferenceManager->copy = internal_copy;
}

static inline void internal_create_typed_ref (void* object, void* type, void* value) {
    JITINT32 fieldOffsetType;
    JITINT32 fieldOffsetValue;
    void*                   typeValue;

    /* Initialize the variables			*/
    fieldOffsetType = 0;
    fieldOffsetValue = 0;

    /* Check the metadata				*/
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /* Fetch the length offset	*/
    fieldOffsetType = (cliManager->CLR).typedReferenceManager.typeFieldOffset;
    fieldOffsetValue = (cliManager->CLR).typedReferenceManager.valueFieldOffset;
    assert(fieldOffsetType >= 0);
    assert(fieldOffsetValue >= 0);

    typeValue = (&((cliManager->CLR).runtimeHandleManager))->getRuntimeTypeHandleValue(&((cliManager->CLR).runtimeHandleManager), type);

    //(*((void*)(object+fieldOffset)))=type;
    memcpy(object + fieldOffsetType, &typeValue, sizeof(void *));
    memcpy(object + fieldOffsetValue, &value, sizeof(void *));

    return;
}

static inline void internal_check_metadata_typedref (void) {

    /* Check if we have to load the metadata	*/
    if ((cliManager->CLR).typedReferenceManager.typedReferenceType == NULL) {

        /* Fetch the System.typedReference type			*/
        (cliManager->CLR).typedReferenceManager.typedReferenceType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "TypedReference");

        /* Fetch the type field ID			*/
        (cliManager->CLR).typedReferenceManager.typeField = (cliManager->CLR).typedReferenceManager.typedReferenceType->getFieldFromName((cliManager->CLR).typedReferenceManager.typedReferenceType, (JITINT8 *) "type");

        /* Fetch the offset				*/
        (cliManager->CLR).typedReferenceManager.typeFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).typedReferenceManager.typeField);

        /* Fetch the value field ID			*/
        (cliManager->CLR).typedReferenceManager.valueField = (cliManager->CLR).typedReferenceManager.typedReferenceType->getFieldFromName((cliManager->CLR).typedReferenceManager.typedReferenceType, (JITINT8 *) "value");

        /* Fetch the offset				*/
        (cliManager->CLR).typedReferenceManager.valueFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).typedReferenceManager.valueField);

        (cliManager->CLR).typedReferenceManager.typedReferenceArray = (cliManager->CLR).typedReferenceManager.typedReferenceType->makeArrayDescriptor((cliManager->CLR).typedReferenceManager.typedReferenceType, 1);
    }

    /* Final assertions				*/
    assert((cliManager->CLR).typedReferenceManager.typeField != NULL);
    assert((cliManager->CLR).typedReferenceManager.valueField != NULL);
    assert((cliManager->CLR).typedReferenceManager.typedReferenceType != NULL);

    /* Return					*/
    return;
}

static inline JITINT32 internal_getTypeOffset (t_typedReferenceManager *self) {

    /*	assertions		*/
    assert(self != NULL);

    /*	get the offsets		*/
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /*	return			*/
    return self->typeFieldOffset;
}

static inline JITINT32 internal_getValueOffset (t_typedReferenceManager *self) {

    /*	assertions		*/
    assert(self != NULL);

    /*	get the offsets		*/
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /*	return			*/
    return self->valueFieldOffset;
}

/**
 * \brief Given a typedReference, returns its "value" field
 *
 * @param thisTypedRef the typedReference we want to extract the value from
 */
static inline void *internal_getValue (void *thisTypedRef) {
    void            *value;

    /*	get the offsets		*/
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /*	return			*/
    value = thisTypedRef + internal_getValueOffset(&((cliManager->CLR).typedReferenceManager));
    return value;
}

/**
 * \brief Given a typedReference, returns its "type" field
 *
 * @param thisTypedRef the typedReference we want to extract the type from
 */
static inline void * internal_getType (void *thisTypedRef) {
    void            *type;

    /*	get the offsets		*/
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /*	return			*/
    type = thisTypedRef + internal_getTypeOffset(&((cliManager->CLR).typedReferenceManager));
    return type;
}

/**
 * \brief Sets the "type" field of a typedReference
 *
 * @param thisTypedRef the typedReference we want to extract the type from.
 * @param type the type we want to set the typedReference to.
 */
static inline void internal_setType (void *thisTypedRef, void *type) {

    /*	get the offsets		*/
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /*	return			*/
    *((void **) (thisTypedRef + internal_getTypeOffset(&((cliManager->CLR).typedReferenceManager)))) = type;
}

/**
 * \brief Sets the "value" field of a typedReference
 *
 * @param thisTypedRef the typedReference we want to extract the value from.
 * @param type the value we want to store into the typedReference.
 */
static inline void internal_setValue (void *thisTypedRef, void *value) {

    /*	get the offsets		*/
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /*	return			*/
    *((void **) (thisTypedRef + internal_getValueOffset(&((cliManager->CLR).typedReferenceManager)))) = value;
}

static inline TypeDescriptor *internal_fillILTypedRefType () {

    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /* Return				*/
    return (cliManager->CLR).typedReferenceManager.typedReferenceType;
}

static inline TypeDescriptor *internal_fillILTypedRefArray () {

    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);

    /* Return				*/
    return (cliManager->CLR).typedReferenceManager.typedReferenceArray;
}

static inline void internal_copy (void *dest, void *src) {
    internal_setValue(dest, internal_getValue(src));
    internal_setType(dest, internal_getType(src));
}

static inline void internal_typedReference_initialize (void) {
    PLATFORM_pthread_once(&typedRefMetadataLock, internal_check_metadata_typedref);
}
