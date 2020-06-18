/*
 * Copyright (C) 2006-2009 Campanoni Simone
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
#ifndef INTERNAL_CALLS_MISC_H
#define INTERNAL_CALLS_MISC_H

#include <jitsystem.h>

/****************************************************************************************************************************
*                                               BUFFER									    *
****************************************************************************************************************************/
void            System_Buffer_Copy (void *src, JITINT32 srcOffset, void *dst, JITINT32 dstOffset, JITINT32 count);
JITINT32        System_Buffer_GetLength (void *array);
JITUINT8        System_Buffer_GetElement (void *array, JITINT32 index);
void            System_Buffer_SetElement (void *array, JITINT32 index, JITUINT8 value);

/**
 * Copy data between arrays
 *
 * This is a Mono internal call. Copy count bytes from source to destination,
 * starting reading from sourceOffset, and starting writing at
 * destinationOffset.
 *
 * @param source data to be copied
 * @param sourceOffset where to start reading data to be copied
 * @param destination where to write data
 * @param destinationOffset where to start writing data
 * @param count how many bytes to copy
 *
 * @return JITFALSE on errors, JITTRUE otherwise
 */
JITBOOLEAN System_Buffer_BlockCopyInternal (void* source,
        JITINT32 sourceOffset,
        void* destination,
        JITINT32 destinationOffset,
        JITINT32 count);

/**
 * Gets the length of the array in bytes
 *
 * @return the length of array in bytes
 */
JITINT32 System_Buffer_ByteLengthInternal (void* array);

/****************************************************************************************************************************
*                                               GLOBALIZATION								    *
****************************************************************************************************************************/
JITINT32        System_Globalization_CultureInfo_InternalCultureID (void);
void *          System_Globalization_CultureInfo_InternalCultureName (void);

/**
 * Initialize locale specific fields of the self object
 *
 * This is a Mono internal call.
 *
 * @param self like this in C++
 * @param name the name of the locale to be used to initialize the self culture
 *
 * @return JITTRUE on success, JITFALSE is name is NULL
 */
JITBOOLEAN
System_Globalization_CultureInfo_construct_internal_locale_from_name (
    void* self,
    void* name);

/**
 * Initialize the numInfo field of the self object
 *
 * This is a Mono internal call.
 *
 * @param self like this in C++
 */
void System_Globalization_CultureInfo_construct_number_format (void* self);

/**
 * Initialize the self object using the given lcid
 *
 * This is a Mono internal call.
 *
 * @param self like this in C++
 * @param lcid identify the locale to use to initialize self
 *
 * @return JITTRUE
 */
JITBOOLEAN
System_Globalization_CultureInfo_construct_internal_locale_from_lcid (
    void* self,
    JITINT32 lcid);


/****************************************************************************************************************************
*                                               PLATFORM								    *
****************************************************************************************************************************/
JITINT32                Platform_TimeMethods_GetUpTime (void);
JITINT64                Platform_TimeMethods_GetCurrentTime (void);
JITINT64                Platform_TimeMethods_GetCurrentUtcTime (void);
JITINT32                Platform_TimeMethods_GetTimeZoneAdjust (JITINT64 *time);
JITINT32                Platform_TaskMethods_GetEnvironmentCount (void);
void *                  Platform_TaskMethods_GetEnvironmentKey (JITINT32 posn);
void *                  Platform_TaskMethods_GetEnvironmentValue (JITINT32 posn);
void                    Platform_TaskMethods_Exit (JITINT32 exitCode);
void *                  Platform_TaskMethods_GetEnvironmentVariable (void *name);
void *                  Platform_TaskMethods_GetCommandLineArgs (void);
void *                  Platform_InfoMethods_GetRuntimeVersion (void);
/****************************************************************************************************************************
*                                               BITCONVERTER								    *
****************************************************************************************************************************/
JITINT64        System_BitConverter_DoubleToInt64Bits (JITFLOAT64 value);
JITBOOLEAN              System_BitConverter_GetLittleEndian (void);
JITFLOAT64      System_BitConverter_Int64BitsToDouble (JITINT64 value);
JITFLOAT32      System_BitConverter_Int32BitsToFloat (JITINT32 value);
JITINT32        System_BitConverter_FloatToInt32Bits (JITFLOAT32 value);

/****************************************************************************************************************************
*                                               TYPEDREFERENCE								    *
****************************************************************************************************************************/
void * System_TypedReference_ToObject (void *typedRef);

#endif
