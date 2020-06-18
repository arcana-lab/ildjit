/*
 * Copyright (C) 2009 - 2012  Campanoni Simone
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
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>
#include <clr_interface.h>

// My headers
#include <lib_internalCall.h>
#include <clr_system.h>
// End

#define INFORMATIONS                    "This plugin implements the base runtime for the ILDJIT framework"
#define AUTHOR                          "Simone Campanoni"

static inline void clrbase_init (t_system *system);
static inline char * clrbase_getName (void);
static inline void clrbase_shutdown (void);
static inline char * clrbase_get_version (void);
static inline char * clrbase_get_information (void);
static inline char * clrbase_get_author (void);
static inline void clrbase_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline void clrbase_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline JITBOOLEAN clrbase_buildMethod (Method method);
static inline JITBOOLEAN clrbase_getNameAndFunctionPointerOfMethod (JITINT8 *signatureInString, JITINT8 **cFunctionName, void **cFunctionPointer);

extern t_system *ildjitSystem;
XanHashTable *filesOpened   = NULL;

clr_interface_t plugin_interface = {
    clrbase_init					,
    clrbase_getName					,
    clrbase_buildMethod				,
    clrbase_getNameAndFunctionPointerOfMethod	,
    clrbase_get_version				,
    clrbase_get_information				,
    clrbase_get_author				,
    clrbase_getCompilationFlags			,
    clrbase_getCompilationTime			,
    clrbase_shutdown
};

static inline void clrbase_init (t_system *system) {
    filesOpened   = xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    return ;
}

static inline char * clrbase_getName (void) {
    return "base";
}

static inline JITBOOLEAN clrbase_buildMethod (Method method) {
    return CLRBASE_buildMethod(method);
}

static inline JITBOOLEAN clrbase_getNameAndFunctionPointerOfMethod (JITINT8 *signatureInString, JITINT8 **cFunctionName, void **cFunctionPointer) {
    JITBOOLEAN	found;

    (*cFunctionName)	= NULL;
    (*cFunctionPointer)	= NULL;

    CLRBASE_getNameAndFunctionPointerOfMethod(signatureInString, cFunctionName, cFunctionPointer);

    found	= ((*cFunctionName) != NULL) && ((*cFunctionPointer) != NULL);

    return found;
}

static inline void clrbase_shutdown (void) {
    XanHashTableItem    *item;

    item    = xanHashTable_first(filesOpened);
    while (item != NULL){
        file_t  *f;
        f       = item->element;
        assert(f != NULL);
        PLATFORM_close(f->fileDescriptor);
        item    = xanHashTable_next(filesOpened, item);
    }
    xanHashTable_destroyTableAndData(filesOpened);
    filesOpened = NULL;

    return ;
}

static inline char * clrbase_get_version (void) {
    return VERSION;
}

static inline char * clrbase_get_information (void) {
    return INFORMATIONS;
}

static inline char * clrbase_get_author (void) {
    return AUTHOR;
}

static inline void clrbase_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

static inline void clrbase_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
