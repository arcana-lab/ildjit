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
   *@file iljit-utils.h
 */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <platform_API.h>

// My headers
#include <ecma_constant.h>
#include <jitsystem.h>
// End

/* Optimizator flags		*/
#define CONTINUOUS_OPTIMIZATION         1
#define ILClockID                       clockid_t

#define combineHash(seed, hash_value) (seed ^ ((JITUINT32) hash_value + 0x9e3779b9 + ((JITUINT32) seed << 6) + ((JITUINT32) seed >> 2)))

#define gettid() PLATFORM_getThisThreadId()

#define JOB_START ((JITUINT32) 0)
#define JOB_END ((JITUINT32) ~0)

typedef struct t_binary_reader {
    FILE		*stream;
    JITUINT64	offset;			/**< Current offset of the file				*/
    JITUINT64	relative_offset;	/**< Current offset from some start point of the file	*/
} t_binary_reader;

/**
 * @brief Binary information
 *
 * This struct contains all information needed about the assembly file
 */
typedef struct t_binary {
    pthread_mutex_t mutex;			/** Lock the mutex of the binary			*/
    t_binary_reader	**reader;		/** Structure used for read data from the binary, make	*/
    /** of PMP						*/
    JITUINT64	metadata_start_point;	/**< Offset from the begin of the file to the begin of	*
						  *  the metadata informations				*/
} t_binary;

typedef struct _DescriptorMetadataTicket {
    struct _DescriptorMetadataTicket *next;
    JITUINT32 type;
    void *data;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} DescriptorMetadataTicket;


typedef struct _DescriptorMetadata {
    pthread_mutex_t mutex;

    struct _DescriptorMetadataTicket *next;
} DescriptorMetadata;

JITUINT32 ILDJIT_numberOfDigits (JITUINT64 v);
JITUINT32 ILDJIT_maxNumberOfDigits (JITUINT16 irType);
JITUINT32 ILDJIT_writeIntegerValueInMemory (void *mem, JITUINT32 offset, JITUINT32 memSize, JITINT32 v, JITBOOLEAN appendNewLine);
JITUINT32 ILDJIT_writeStringInMemory (void *mem, JITUINT32 offset, JITUINT32 memSize, JITINT8 *stringToWrite, JITBOOLEAN appendNewLine);
JITUINT32 ILDJIT_readIntegerValueFromMemory (void *mem, JITUINT32 offset, JITINT32 *v);
JITUINT32 ILDJIT_readStringFromMemory (void *mem, JITUINT32 offset, JITINT8 **outputString);

DescriptorMetadataTicket *createDescriptorMetadataTicket (DescriptorMetadata *metadata, JITUINT32 type);
void broadcastDescriptorMetadataTicket (DescriptorMetadataTicket *next, void *data);
void init_DescriptorMetadata (DescriptorMetadata *metadata);

/**
 *
 * Print binary offset
 */
void print_binary_offset (t_binary *binary);

/** Reads a given number of bytes from a buffer, at a given offset, storing the result in the output variable */
#define read_from_buffer(bufferAddress, offset, lengthInBytes, outputAddress) \
	{	\
		*outputAddress = 0;	  \
		memcpy(outputAddress, bufferAddress + offset, lengthInBytes);	  \
	}

void readline (JITINT8 **line, FILE *fileToRead);

/**
 *
 * Read byte bytes from the binary pointed by the binary parameter and stores them into the memory pointed by the buffer parameter
 */
JITINT32 il_read (JITINT8 *buffer, JITINT32 byte, t_binary *binary);

/**
 *
 * Seek the file binary to the dest_offset
 */
JITINT32 seek_within_file (t_binary *binary, JITINT8 *buffer, JITUINT32 current_offset, JITUINT32 dest_offset);

/**
 *
 * Return the binary identificator in a string format
 */
char * architecture_ID_to_string (JITUINT16 architecture_ID);

/**
 *
 * Return the section characteristics in a string format
 */
char * section_characteristic_to_string (JITUINT32 section_characteristic);

/**
 *
 * Return the flags in a string format
 */
char * cli_flags_to_string (JITUINT32 flags);

/**
 *
 * Seek the binary at the first byte of the file
 */
JITINT32 unroll_file (t_binary *binary);

/**
 *
 * Return 0 if the blob index is 16 bit, return 1 otherwise
 */
JITBOOLEAN blob_index_is_32 (JITINT32 heap_size);

/**
 *
 * Return 0 if the string index is 16 bit, return 1 otherwise
 */
JITBOOLEAN string_index_is_32 (JITINT32 heap_size);

/**
 *
 * Return 0 if the guid index is 16 bit, return 1 otherwise
 */
JITBOOLEAN guid_index_is_32 (JITINT32 heap_size);

/**
 *
 * Set the offset from the start point of the file, where the metadata starts
 */
void set_metadata_start_point (t_binary *binary, JITUINT32 offset);

/**
 *
 * Return the offset from the begin of the file, where the metadata starts
 */
JITUINT64 get_metadata_start_point (t_binary *binary);

/**
 *
 * Return the float value from the seconds and the nanoseconds parameters
 */
JITFLOAT32 to_float (JITUINT32 seconds, JITINT64 nanoseconds);

JITFLOAT32 diff_time (struct timespec startTime, struct timespec stopTime);
/**
 *
 * Setup prefix to match the one cotaining the requested library
 */
char * setupPrefixFromPath (char *path, char *library_name, char *prefix);

JITINT8 * fetchLastName (JITINT8 *name);

/**
 *
 * Return the best clock identification of the running system
 */
ILClockID getBestClockID ();

JITUINT32 getIRSize (JITUINT32 IRType);

JITBOOLEAN is_byReference_or_Object_IRType (JITUINT32 IRType);

JITUINT32 simpleHash (void *value);

JITUINT32 hashString (void *str);
JITINT32 equalsString (void *str1, void* str2);

JITUINT32 hashStringIgnoreCase (void *str);
JITINT32 equalsStringIgnoreCase (void *str1, void *str2);
void getFirstPath (JITINT8 *paths, JITINT8 *path);

/**
 *
 * Return the version of the Libiljitu library
 */
char * libiljituVersion ();

/**
 *
 * Write the compilation flags to the buffer given as input.
 */
void libiljituCompilationFlags (char *buffer, JITINT32 bufferLength);

/**
 *
 * Write the compilation time to the buffer given as input.
 */
void libiljituCompilationTime (char *buffer, JITINT32 bufferLength);


#define SNPRINTF(str, size, format, args ...) snprintf((char *) str, size, (char *) format, ## args)
#define STRLEN(s) strlen((const char *) s)
#define STRNCMP(s1, s2, n) strncmp((const char *) s1, (const char *) s2, n)
#define STRCMP(dest, src) strcmp((const char *) dest, (const char *) src)
#define STRNCAT(dest, src, n) (JITINT8 *) strncat((char *) dest, (char *) src, n)
#define STRCAT(dest, src) (JITINT8 *) strcat((char *) dest, (char *) src)
#define STRNCPY(dest, src, n) strncpy((char *) dest, (char *) src, n)
#define STRCPY(dest, src) strcpy((char *) dest, (char *) src)
#define PRINT_ERR(message, err) print_err((char *) message, err)

#endif
