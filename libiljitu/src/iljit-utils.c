/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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
/**
   *@file iljit-utils.c
 */
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <platform_API.h>

#include <xanlib.h>
#include <compiler_memory_manager.h>
#ifdef PROFILE_CLOCKGETTIME
#include <time.h>
#endif

// My headers
#include <iljit-utils.h>
#include <iljitu-system.h>
#include <jitsystem.h>
#include <config.h>
#include <xanlib.h>
#include <ir_language.h>
// End

#define DIM_BUF 2056

static inline void get_first_path (char *paths, char *path);

JITUINT32 ILDJIT_writeIntegerValueInMemory (void *mem, JITUINT32 offset, JITUINT32 memSize, JITINT32 v, JITBOOLEAN appendNewLine) {
    JITUINT32	bytesWritten;

    bytesWritten		= 0;
    if (appendNewLine) {
        snprintf((void *)(((JITNUINT)(mem)) + offset), memSize - offset, "%d\n", v);
        bytesWritten++;
    } else {
        snprintf((void *)(((JITNUINT)(mem)) + offset), memSize - offset, "%d", v);
    }
    bytesWritten		+= ILDJIT_numberOfDigits(v);

    return bytesWritten;
}

JITUINT32 ILDJIT_writeStringInMemory (void *mem, JITUINT32 offset, JITUINT32 memSize, JITINT8 *stringToWrite, JITBOOLEAN appendNewLine) {
    JITUINT32	bytesWritten;

    bytesWritten		= strlen((char *)stringToWrite);
    if (appendNewLine) {
        snprintf((void *)(((JITNUINT)(mem)) + offset), memSize - offset, "%s\n", (char *)stringToWrite);
        bytesWritten++;
    } else {
        snprintf((void *)(((JITNUINT)(mem)) + offset), memSize - offset, "%s", (char *)stringToWrite);
    }

    return bytesWritten;
}

JITUINT32 ILDJIT_readIntegerValueFromMemory (void *mem, JITUINT32 offset, JITINT32 *v) {
    JITUINT32	bytesRead;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(v != NULL);

    /* Read the integer value.
     */
    (*v)		= atoi((char *)(((JITNUINT)mem) + offset));
    bytesRead	= ILDJIT_numberOfDigits(*v);

    /* Remove the next new line if it exist.
     */
    if ((*((char *)(((JITNUINT)mem) + offset + bytesRead))) == '\n') {
        bytesRead++;
    }

    return bytesRead;
}

JITUINT32 ILDJIT_readStringFromMemory (void *mem, JITUINT32 offset, JITINT8 **outputString) {
    char		*s;
    JITUINT32	sLength;
    JITUINT32	count;

    /* Fetch the string.
     */
    s	= (char *)(((JITNUINT)mem) + offset);
    assert(s != NULL);

    /* Fetch the length of the string.
     */
    sLength	= strlen(s);
    for (count=0; count < sLength; count++) {
        if (	(s[count] == '\0')	||
                (s[count] == '\n')	) {
            break ;
        }
    }
    sLength	= count;

    /* Copy the string.
     */
    (*outputString)	= allocFunction(sLength + 1);
    SNPRINTF(*outputString, sLength + 1, "%s", s);

    /* Remove the next new line if it exist.
     */
    if ((*((char *)(((JITNUINT)mem) + offset + sLength))) == '\n') {
        sLength++;
    }

    return sLength;
}

JITUINT32 ILDJIT_numberOfDigits (JITUINT64 v) {
    return (JITUINT32) floorl(log10l(llabs (v))) + 1;
}

JITUINT32 ILDJIT_maxNumberOfDigits (JITUINT16 irType) {
    JITUINT32	digits;

    digits	= 0;
    switch(irType) {
        case IRNINT:
            if (sizeof(JITNINT) == 4) {
                irType	= IRINT32;
            } else {
                irType	= IRINT64;
            }
            break;

        case IRNUINT:
            if (sizeof(JITNUINT) == 4) {
                irType	= IRUINT32;
            } else {
                irType	= IRUINT64;
            }
            break;
    }
    switch (irType) {
        case IRINT32:
        case IRUINT32:
            digits	= ILDJIT_numberOfDigits(JITMAXUINT32);
            break ;
        case IRINT64:
        case IRUINT64:
            digits	= ILDJIT_numberOfDigits(JITMAXUINT64);
            break ;
    }

    return digits;
}

char * cli_flags_to_string (JITUINT32 flags) {
    char *buffer = (char *) allocMemory(sizeof(char) * DIM_BUF);

    if (buffer == NULL) {
        return NULL;
    }
    buffer[0] = '\0';
    if ((flags & COMIMAGE_FLAGS_32BITREQUIRED) != 0) {
        strncat(buffer, "32bit ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    if ((flags & COMIMAGE_FLAGS_STRONGNAMESIGNED) != 0) {
        strncat(buffer, "StrongNameSigned ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    return buffer;
}

char * section_characteristic_to_string (JITUINT32 section_characteristic) {
    char *buffer = (char *) allocMemory(sizeof(char) * DIM_BUF);

    if (buffer == NULL) {
        return NULL;
    }
    strncpy(buffer, "This section can be ", sizeof(char) * DIM_BUF);
    if ((section_characteristic & 0x80000000) != 0) {
        strncat(buffer, "Written ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    if ((section_characteristic & 0x40000000) != 0) {
        strncat(buffer, "Read ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    if ((section_characteristic & 0x20000000) != 0) {
        strncat(buffer, "Execute as code ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    strncat(buffer, " and it contain ", sizeof(char) * DIM_BUF - sizeof(buffer));
    if ((section_characteristic & 0x00000020) != 0) {
        strncat(buffer, "Executable code ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    if ((section_characteristic & 0x00000040) != 0) {
        strncat(buffer, "Initialized data ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    if ((section_characteristic & 0x00000020) != 0) {
        strncat(buffer, "Unitialized data ", sizeof(char) * DIM_BUF - sizeof(buffer));
    }
    return buffer;
}

char * architecture_ID_to_string (JITUINT16 architecture_ID) {
    switch (architecture_ID) {
        case 332:
            return "Intel 386";
        default:
            return "Not know";
    }
}

JITINT32 il_read (JITINT8 *buffer, JITINT32 byte, t_binary *binary) {
    JITINT32 bytesRead;

    /* Assertions           */
    assert(binary				!= NULL);
    assert(binary->reader			!= NULL);
    assert(*(binary->reader)		!= NULL);
    assert((*(binary->reader))->stream	!= NULL);
    assert(buffer				!= NULL);

    /* Read from file       */
    bytesRead       = fread(buffer, sizeof(char), byte, (*(binary->reader))->stream);

    /* Check the read       */
    if (bytesRead != byte) {
        if (feof((*(binary->reader))->stream)) {
            print_err("ILJIT-UTILS: End-of-file", 0);
        } else {
            print_err("ILJIT-UTILS: ERROR during reading the binary. ", 0);
        }
        clearerr((*(binary->reader))->stream);
        abort();
    }

    /* Update the offset    */
    (*(binary->reader))->offset		+= byte;
    (*(binary->reader))->relative_offset	+= byte;

    /* Return               */
    return 0;
}

void print_binary_offset (t_binary *binary) {
    fprintf(stderr, "ILJIT UTILS: File offset               =       %d\n", (JITUINT32) (*(binary->reader))->offset);
    fprintf(stderr, "ILJIT UTILS: File relative offset      =       %d\n", (JITUINT32) (*(binary->reader))->relative_offset);
}

JITINT32 unroll_file (t_binary *binary) {
    (*(binary->reader))->offset = 0;
    (*(binary->reader))->relative_offset = 0;
    if (fseek((*(binary->reader))->stream, 0, SEEK_SET) != 0) {
        print_err("ERROR: During seeking the file into the begin. ", errno);
        return -1;
    }
    return 0;
}

JITINT32 seek_within_file (t_binary *binary, JITINT8 *buffer, JITUINT32 current_offset, JITUINT32 dest_offset) {
    JITUINT32 size;

    /* Assertions			*/
    assert(binary != NULL);
    assert(buffer != NULL);

    if (current_offset == dest_offset) {
        return 0;
    }
    if (current_offset > dest_offset) {
        unroll_file(binary);
        current_offset = 0;
    }
    assert(current_offset < dest_offset);

    while (current_offset < dest_offset) {
        if ((dest_offset - current_offset) > 1024) {
            size = 1024;
        } else {
            size = (JITUINT32) (dest_offset - current_offset);
        }
        if (il_read(buffer, size, binary) != 0) {
            return -1;
        }
        current_offset += size;
    }
    return (current_offset != dest_offset) ? 1 : 0;
}

JITBOOLEAN blob_index_is_32 (JITINT32 heap_size) {
    if (((heap_size >> BLOB_STREAM_HEAP_SIZE_EXTENDED_BIT) & 0x1) == 0) {
        return 0;
    }
    return 1;
}

JITBOOLEAN string_index_is_32 (JITINT32 heap_size) {
    if (((heap_size >> STRING_STREAM_HEAP_SIZE_EXTENDED_BIT) & 0x1) == 0) {
        return 0;
    }
    return 1;
}

JITBOOLEAN guid_index_is_32 (JITINT32 heap_size) {
    if (( (heap_size >> GUID_STREAM_HEAP_SIZE_EXTENDED_BIT) & 0X1 ) == 0) {
        return 0;
    }
    return 1;
}

void set_metadata_start_point (t_binary *binary, JITUINT32 offset) {
    binary->metadata_start_point = offset;
}

JITUINT64 get_metadata_start_point (t_binary *binary) {
    return binary->metadata_start_point;
}

JITFLOAT32 to_float (JITUINT32 seconds, JITINT64 nanoseconds) {
    JITFLOAT32 newTime;

    newTime = ((JITFLOAT32) seconds) + (((JITFLOAT32) nanoseconds) / ((JITFLOAT32) 1000000000));
    return newTime;
}

char * setupPrefixFromPath (char *path, char *library_name, char *prefix) {
    char		temp_name[strlen(path) + strlen(library_name) + 6];
    JITUINT32	length;
    JITUINT32	library_name_len;
    JITBOOLEAN	suffixInPlace;

    /* Assertions			*/
    assert(path		!= NULL);
    assert(library_name	!= NULL);
    assert(prefix		!= NULL);

    /* Check if the library name includes already the suffix.
     */
    suffixInPlace		= JITFALSE;
    library_name_len	= strlen(library_name);
    if (	(library_name[library_name_len - 4] == '.')	&&
            (library_name[library_name_len - 3] == 'd')	&&
            (library_name[library_name_len - 2] == 'l')	&&
            (library_name[library_name_len - 1] == 'l')	) {
        suffixInPlace	= JITTRUE;
    }

    /* Make the name of the		*
     * library			*/
    get_first_path(path, prefix);
    if (strlen(prefix) == 0) {
        length = strlen(prefix) + strlen(library_name) + 5;
        snprintf(temp_name, sizeof(char) * length, "%s%s", library_name, suffixInPlace ? "" : ".dll");
    } else {
        length = strlen(prefix) + strlen(library_name) + 6;
        snprintf(temp_name, sizeof(char) * length, "%s/%s%s", prefix, library_name, suffixInPlace ? "" : ".dll");
    }

    /* Try access the library	*/
    if ( access(temp_name, R_OK) == 0 ) {
        if (realpath(temp_name, prefix) == NULL) {
            print_err("ERROR: realpath returns NULL. ", errno);
            abort();
        }
        *(strrchr(prefix, '/')) = '\0';
        return prefix;
    } else {
        if (strlen(path) > strlen(prefix)) {
            path    += strlen(prefix) + 1;
            return setupPrefixFromPath(path, library_name, prefix);
        } else {
            prefix[0] = '\0';
        }
    }

    /* Return			*/
    return NULL;
}

void getFirstPath (JITINT8 *paths, JITINT8 *path) {
    get_first_path((char *) paths, (char *) path);
}

static inline void get_first_path (char *paths, char *path) {
    JITUINT32 count;

    /* Assertions		*/
    assert(paths != NULL);

    for (count = 0; count < strlen(paths); count++) {
        path[count] = paths[count];
        if (path[count] == ENV_VAR_SEPARATOR) {
            path[count] = '\0';
            break;
        }
    }
    path[count] = '\0';
}

void libiljituCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf((char *) buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat((char *) buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat((char *) buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat((char *) buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

void libiljituCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libiljituVersion () {
    return VERSION;
}

ILClockID getBestClockID () {
    ILClockID championID;

#ifdef PROFILE_CLOCKGETTIME
    struct timespec N;

    championID = -1;
    if (clock_getres(CLOCK_PROCESS_CPUTIME_ID, &N) == 0) {
        championID = CLOCK_PROCESS_CPUTIME_ID;
    } else if (clock_getres(CLOCK_REALTIME, &N) == 0) {
        championID = CLOCK_REALTIME;
    }

#else
    championID = -1;
#endif
    return championID;
}

JITFLOAT32 diff_time (struct timespec startTime, struct timespec stopTime) {
    JITFLOAT32 timeElapsed;

    /* Initialize the variables	*/
    timeElapsed = 0;

    timeElapsed = stopTime.tv_sec + stopTime.tv_nsec / 1E9;
    timeElapsed -= startTime.tv_sec + startTime.tv_nsec / 1E9;

    /* Return			*/
    return timeElapsed;
}

JITINT8 * fetchLastName (JITINT8 *name) {
    JITUINT32 count;
    JITUINT32 startPoint;
    JITUINT32 length;
    JITINT8         *lastName;

    /* Assertions		*/
    assert(name != NULL);

    length = STRLEN(name);
    lastName = name;
    startPoint = 0;

    for (count = 1; count < length; count++) {
        if (    (name[count] == (JITINT8) '.')         &&
                (startPoint != count )       ) {
            startPoint = count + 1;
        }
    }

    return lastName + startPoint;
}

JITUINT32 getIRSize (JITUINT32 IRType) {
    switch (IRType) {
        case IRVOID:
            return 0;
        case IRINT8:
        case IRUINT8:
            return 1;
        case IRINT16:
        case IRUINT16:
            return 2;
        case IRINT32:
        case IRUINT32:
        case IRFLOAT32:
            return 4;
        case IRINT64:
        case IRUINT64:
        case IRFLOAT64:
            return 8;
        case IRNFLOAT:
            return sizeof(JITNFLOAT);
        case IRNUINT:
            return sizeof(JITNUINT);
        case IRNINT:
            return sizeof(JITNINT);
        case IRTPOINTER:
        case IRMPOINTER:
        case IROBJECT:
            return sizeof(JITNUINT);
        case IRVALUETYPE:
            print_err("getIRSize: not enough information. ", 0);
            abort();
        default:
            print_err("getIRSize: Type is not known. ", 0);
            abort();
    }
}

JITBOOLEAN is_byReference_or_Object_IRType (JITUINT32 IRType) {
    return (IRType == IRMPOINTER) || (IRType == IRTPOINTER) || (IRType == IRNINT) || (IRType == IROBJECT);
}

/* DescriptorMetadata */
void init_DescriptorMetadata (DescriptorMetadata *metadata) {
    assert(metadata != NULL);
    metadata->next = NULL;
    pthread_mutexattr_t mutex_attr;

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&(metadata->mutex), &mutex_attr);
}

DescriptorMetadataTicket *createDescriptorMetadataTicket (DescriptorMetadata *metadata, JITUINT32 type) {
    PLATFORM_lockMutex(&(metadata->mutex));

    /* lookup for existing ticket */
    DescriptorMetadataTicket *ticket = metadata->next;
    while (ticket != NULL) {
        if (ticket->type == type) {
            break;
        }
        ticket = ticket->next;
    }

    if (ticket == NULL) {
        /* create new ticket */
        ticket = sharedAllocFunction(sizeof(DescriptorMetadataTicket));
        pthread_mutexattr_t mutex_attr;
        pthread_condattr_t cond_attr;

        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_initCondVarAttr(&cond_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        MEMORY_shareConditionVariable(&cond_attr);
        PLATFORM_initMutex(&(ticket->mutex), &mutex_attr);
        PLATFORM_initCondVar(&(ticket->cond), &cond_attr);

        ticket->type = type;
        ticket->data = NULL;
        ticket->next = metadata->next;
        metadata->next = ticket;

        PLATFORM_unlockMutex(&(metadata->mutex));
    } else {
        PLATFORM_unlockMutex(&(metadata->mutex));

        /* wait if neeed */
        PLATFORM_lockMutex(&(ticket->mutex));
        if (ticket->data == NULL) {
            PLATFORM_waitCondVar(&(ticket->cond), &(ticket->mutex));
        }
        PLATFORM_unlockMutex(&(ticket->mutex));
    }
    return ticket;
}

void broadcastDescriptorMetadataTicket (DescriptorMetadataTicket *ticket, void *data) {
    PLATFORM_lockMutex(&(ticket->mutex));
    ticket->data = data;
    PLATFORM_unlockMutex(&(ticket->mutex));
    PLATFORM_broadcastCondVar(&(ticket->cond));
}

JITUINT32 simpleHash (void *value) {
    JITUINT32 i = (JITUINT32) (JITNUINT)value;

    i += ~(i << 9);
    i ^= ((i >> 14) | (i << 18));
    i += (i << 4);
    i ^= ((i >> 10) | (i << 22));
    return i;
}

JITUINT32 hashString (void *string) {
    JITUINT32 hval;
    JITUINT32 g;

    JITINT8 *str = (JITINT8 *) string;

    /* Compute the hash value for the given string.  */
    hval = 0;
    while (*str != '\0') {
        hval <<= 4;
        hval += (JITUINT32) *str++;
        g = hval & ((JITUINT32) 0xf << 28);
        if (g != 0) {
            hval ^= g >> 24;
            hval ^= g;
        }
    }
    return hval;
}


JITINT32 equalsString (void *str1, void* str2) {
    if (str1 == NULL || str2 == NULL) {
        return 0;
    }
    return STRCMP((JITINT8 *) str1, (JITINT8 *) str2) == 0;
}


JITUINT32 hashStringIgnoreCase (void *string) {
    JITUINT32 hval;
    JITUINT32 g;

    JITINT8 *str = (JITINT8 *) string;

    /* Compute the hash value for the given string.  */
    hval = 0;
    while (*str != '\0') {
        hval <<= 4;
        JITINT8 ch = *str++;
        if (ch >= 'A' && ch <= 'Z') {
            ch = (ch - 'A' + 'a');
        }
        hval += (JITUINT32) ch;
        g = hval & ((JITUINT32) 0xf << 28);
        if (g != 0) {
            hval ^= g >> 24;
            hval ^= g;
        }
    }
    return hval;
}

JITINT32 equalsStringIgnoreCase (void* string1, void *string2) {
    if (string1 == NULL || string2 == NULL) {
        return 0;
    }
    JITINT8 *str1 = (JITINT8 *) string1;
    JITINT8 *str2 = (JITINT8 *) string2;
    while (*str1 != '\0' && *str2 != '\0') {
        JITINT8 ch1 = *str1++;
        if (ch1 >= 'A' && ch1 <= 'Z') {
            ch1 = (ch1 - 'A' + 'a');
        }
        JITINT8 ch2 = *str2++;
        if (ch2 >= 'A' && ch2 <= 'Z') {
            ch2 = (ch2 - 'A' + 'a');
        }
        if ((ch1 < ch2) || (ch1 > ch2)) {
            return 0;
        }
    }
    if ((*str1 != '\0') || (*str2 != '\0')) {
        return 0;
    }
    return 1;
}

void readline (JITINT8 **line, FILE *fileToRead) {
    if (fscanf(fileToRead, "%as", line) == 0) {

    }
}
