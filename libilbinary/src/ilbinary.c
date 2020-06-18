/*
 * Copyright (C) 2012 - 2013  Campanoni Simone
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
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <decoder.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>

/**
 * @brief Read the assembly ID
 *
 * Read the identificator of the file (like the magic number)
 */
static inline JITINT32 readIDExe (FILE *file, JITINT8 *buffer);

t_binary_information * ILBINARY_new (void) {
    t_binary_information	*newBinary;
    pthread_mutexattr_t	mutex_attr;

    /* Allocate a new binary.
     */
    newBinary                               = sharedAllocFunction(sizeof(t_binary_information));
    assert(newBinary			!= NULL);
    memset(newBinary, 0, sizeof(t_binary_information));
    newBinary->prefix			= sharedAllocFunction(PATH_MAX);
    newBinary->binary.reader		= MEMORY_obtainPrivateAddress();
    assert(newBinary->binary.reader		!= NULL);
    assert(*(newBinary->binary.reader)	== NULL);

    /* Allocate space for the private binary reader.
     */
    (*(newBinary->binary.reader)) 		= allocFunction(sizeof(t_binary_reader));

    /* Make the mutex.
     */
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&((newBinary->binary).mutex), &mutex_attr);

    return newBinary;
}

void ILBINARY_setBinaryName (t_binary_information *ilBinary, JITINT8 *name) {
    STRNCPY(ilBinary->name, name, MAX_NAME);

    return ;
}

void ILBINARY_setBinaryFile (t_binary_information *ilBinary, FILE *ilBinaryFile) {
    (*(ilBinary->binary.reader))->stream	= ilBinaryFile;

    return ;
}

JITUINT32 ILBINARY_decodeFileType (t_binary_information *binary_info, JITINT8 binary_ID[2]) {

    /* Assertions.
     */
    assert(binary_info != NULL);
    assert(binary_ID != NULL);

    if (readIDExe((*(binary_info->binary.reader))->stream, binary_ID) != 0) {
        return -1;
    }
    if (fseek((*(binary_info->binary.reader))->stream, 0, SEEK_SET) != 0) {
        return -1;
    }

    return 0;
}

static inline JITINT32 readIDExe (FILE *file, JITINT8 *buffer) {
    if (fread(buffer, sizeof(char), 2, file) < 2) {
        return -1;
    }
    return 0;
}
