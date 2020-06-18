/*
 * Copyright (C) 2006 Campanoni Simone, Di Biagio Andrea
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <ir_language.h>
#include <ildjit_locale.h>
#include <jitsystem.h>
#include <metadata_manager.h>
#include <platform_API.h>
#include <ildjit.h>

// My headers
#include <ildjit_profiler.h>
#include <internal_calls_misc.h>
#include <internal_calls_utilities.h>
// End

static inline JITINT16 isAlpha (JITINT8 ch);
static inline JITINT8 * getCultureName ();
static inline JITINT8 toUpper (JITINT8 ch);
static inline JITINT8 toLower (JITINT8 ch);
extern char **environ;
extern t_system *ildjitSystem;

/**************************************************************************************************************************
                                                         INTERNAL CALLS
 *****************************************************************************************************************************/
void System_Buffer_Copy (void *src, JITINT32 srcOffset, void *dst, JITINT32 dstOffset, JITINT32 count) {

    /* Assertions	*/
    assert(src != NULL);
    assert(dst != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Buffer.Copy");

    /*exceptions part*/
    if ((srcOffset > (ildjitSystem->cliManager).CLR.arrayManager.getArrayLengthInBytes(src)) || srcOffset < 0
            || (dstOffset > (ildjitSystem->cliManager).CLR.arrayManager.getArrayLengthInBytes(dst)) || dstOffset < 0) {

        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
    }
    /*end of exceptions part*/

    memcpy(dst + dstOffset, src + srcOffset, count);

    /* Return               */
    METHOD_END(ildjitSystem, "System.Buffer.Copy");
    return;
}

JITINT32 System_Buffer_GetLength (void* array) {
    t_arrayManager* arrayManager;
    JITINT32 length;

    /* Assertions			*/
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Buffer.GetLength");

    /* Fetch the length of the array*/
    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);
    length = arrayManager->getArrayLengthInBytes(array);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Buffer.GetLength");
    return length;
}

JITUINT8 System_Buffer_GetElement (void *array, JITINT32 index) {
    JITUINT8 value;

    /* Assertions		*/
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Buffer.GetElement");

    value = *(JITUINT8 *) (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, index);

    /* Return               */
    METHOD_END(ildjitSystem, "System.Buffer.GetElement");
    return value;
}

void System_Buffer_SetElement (void *array, JITINT32 index, JITUINT8 value) {

    /* Assertions		*/
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Buffer.SetElement");

    *((JITUINT8 *) (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, index)) = value;

    /* Return               */
    METHOD_END(ildjitSystem, "System.Buffer.SetElement");
    return;
}

/* Copy data between arrays */
JITBOOLEAN System_Buffer_BlockCopyInternal (void* source,
        JITINT32 sourceOffset,
        void* destination,
        JITINT32 destinationOffset,
        JITINT32 count) {
    t_arrayManager* arrayManager;
    JITINT32 sourceLength, destinationLength;
    JITBOOLEAN copyDone;

    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);

    METHOD_BEGIN(ildjitSystem, "System.Buffer.BlockCopyInternal");

    sourceLength = arrayManager->getArrayLengthInBytes(source);
    destinationLength = arrayManager->getArrayLengthInBytes(destination);
    if (sourceOffset + count > sourceLength ||
            destinationOffset + count > destinationLength) {
        copyDone = JITFALSE;
    } else {
        System_Buffer_Copy(source, sourceOffset, destination,
                           destinationOffset, count);
        copyDone = JITTRUE;
    }

    METHOD_END(ildjitSystem, "System.Buffer.BlockCopyInternal");

    return copyDone;
}

/* Get the length of array in bytes */
JITINT32 System_Buffer_ByteLengthInternal (void* array) {
    t_arrayManager* arrayManager;
    JITINT32 length;

    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);

    METHOD_BEGIN(ildjitSystem, "System.Buffer.ByteLengthInternal");

    if (arrayManager->storesPrimitiveTypes(array)) {
        length = System_Buffer_GetLength(array);
    } else {
        length = -1;
    }

    METHOD_END(ildjitSystem, "System.Buffer.BufferLengthInternal");

    return length;
}

JITINT32 System_Globalization_CultureInfo_InternalCultureID (void) {

    METHOD_BEGIN(ildjitSystem, "System.Globalization.CultureInfo.InternalCultureID");

    /* We prefer use the string name	*/

    /* Return				*/
    METHOD_END(ildjitSystem, "System.Globalization.CultureInfo.InternalCultureID");
    return 0;
}

void * System_Globalization_CultureInfo_InternalCultureName (void) {
    JITINT8         *cultureName;
    void            *object;

    METHOD_BEGIN(ildjitSystem, "System.Globalization.CultureInfo.InternalCultureName");

    /* Check if the environment give us the	*
     * culture information			*/
    cultureName = getCultureName();
    if (cultureName == NULL) {
        object = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8 *) "iv", 3);
    } else {
        object = CLIMANAGER_StringManager_newInstanceFromUTF8(cultureName, 3);
    }

    /* Return the default culture		*/
    METHOD_END(ildjitSystem, "System.Globalization.CultureInfo.InternalCultureName");
    return object;
}

void * Platform_InfoMethods_GetRuntimeVersion (void) {
    void            *version;

    METHOD_BEGIN(ildjitSystem, "Platform.InfoMethods.GetRuntimeVersion");

    version = CLIMANAGER_StringManager_newInstanceFromUTF8((JITINT8 *) VERSION, strlen(VERSION));
    assert(version != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.InfoMethods.GetRuntimeVersion");
    return version;
}

void * Platform_TaskMethods_GetEnvironmentValue (JITINT32 posn) {
    JITINT32 	count;
    JITINT32 	len;
    void 		*str;

    METHOD_BEGIN(ildjitSystem, "Platform.TaskMethods.GetEnvironmentValue");
    str	= NULL;
    count 	= Platform_TaskMethods_GetEnvironmentCount();
    if (posn >= 0 && posn < count) {
        JITINT8 *value;
        value = (JITINT8 *) environ[posn];
        len = 0;
        while (value[len] != '\0' && value[len] != '=') {
            ++len;
        }
        if (value[len] == '=') {
            len++;
        }
        str = CLIMANAGER_StringManager_newInstanceFromUTF8(value + len, STRLEN(value) - len);
        assert(str != NULL);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.TaskMethods.GetEnvironmentValue");
    return str;
}

void * Platform_TaskMethods_GetEnvironmentKey (JITINT32 posn) {
    JITINT32 	count;
    JITINT32 	len;
    void 		*str;

    METHOD_BEGIN(ildjitSystem, "Platform.TaskMethods.GetEnvironmentKey");
    str	= NULL;
    count 	= Platform_TaskMethods_GetEnvironmentCount();
    if (posn >= 0 && posn < count) {
        JITINT8 *key;
        key = (JITINT8 *) environ[posn];
        len = 0;
        while (key[len] != '\0' && key[len] != '=') {
            ++len;
        }
        str = CLIMANAGER_StringManager_newInstanceFromUTF8(key, len);
        assert(str != NULL);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.TaskMethods.GetEnvironmentKey");
    return str;
}

JITINT32 Platform_TaskMethods_GetEnvironmentCount (void) {
    JITINT32 count = 0;
    JITINT8 **env = (JITINT8 **) environ;

    while (env && *env != 0) {
        ++count;
        ++env;
    }
    return count;
}

void * Platform_TaskMethods_GetEnvironmentVariable (void *name) {
    void            *object;
    JITINT8         *literalReturn;
    JITINT16        *literalUTF16;
    JITINT8         *literalUTF8;
    JITINT32 length;

    METHOD_BEGIN(ildjitSystem, "Platform.TaskMethods.GetEnvironmentVariable");

    object = NULL;
    length = (ildjitSystem->cliManager).CLR.stringManager.getLength(name);
    literalUTF16 = (ildjitSystem->cliManager).CLR.stringManager.toLiteral(name);
    assert(literalUTF16 != NULL);
    literalUTF8 = allocMemory(length + 1);
    (ildjitSystem->cliManager).CLR.stringManager.fromUTF16ToUTF8(literalUTF16, literalUTF8, length);
    literalReturn = (JITINT8 *) getenv((char *) literalUTF8);
    if (literalReturn != NULL) {
        object = CLIMANAGER_StringManager_newInstanceFromUTF8(literalReturn, strlen((char *) literalReturn));
        assert(object != NULL);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "Platform.TaskMethods.GetEnvironmentVariable");
    return object;
}

void * Platform_TaskMethods_GetCommandLineArgs (void) {
    TypeDescriptor          *stringClassLocated;
    void            *array;
    void            *arg_string;
    JITINT32 	argc;
    JITINT8         **argv;
    JITUINT32 	count;

    METHOD_BEGIN(ildjitSystem, "Platform.TaskMethods.GetCommandLineArgs");

    /* Check whether we already have the arguments in CIL array.
     */
    if ((ildjitSystem->arg).stringArray == NULL) {

        /* Make the arguments.
         */
        assert((ildjitSystem->arg).argc > 0);
        argc 	= (ildjitSystem->arg).argc;
        if (argc > 0) {
            argv = (JITINT8 **) (&((ildjitSystem->arg).argv[0]));
        } else {
            argv = NULL;
        }

        stringClassLocated = (ildjitSystem->cliManager).CLR.stringManager.fillILStringType();
        assert(stringClassLocated != NULL);
        array = (ildjitSystem->garbage_collectors).gc.allocArray(stringClassLocated, 1, argc);
        assert(array != NULL);
        assert((ildjitSystem->garbage_collectors).gc.getArrayLength(array) == argc);
        for (count = 0; count < argc; count++) {
            arg_string = CLIMANAGER_StringManager_newInstanceFromUTF8(argv[count], strlen((char *) argv[count]));
            (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, count, &arg_string);
        }
        assert(array != NULL);

        /* Keep track of the array of arguments.
         */
        (ildjitSystem->arg).stringArray	= array;
    }

    METHOD_END(ildjitSystem, "Platform.TaskMethods.GetCommandLineArgs");
    return (ildjitSystem->arg).stringArray;
}

void Platform_TaskMethods_Exit (JITINT32 exitCode) {
    METHOD_BEGIN(ildjitSystem, "Platform.TaskMethods.Exit");
    METHOD_END(ildjitSystem, "Platform.TaskMethods.Exit");
    ILDJIT_quit(exitCode);

    return ;
}

JITINT32 Platform_TimeMethods_GetTimeZoneAdjust (JITINT64 *time) {
    static long timezone = 0;

    METHOD_BEGIN(ildjitSystem, "Platform.TimeMethods.GetTimeZoneAdjust(long)");

    //FIXME da completare in pnet;

    time_t temp = (time_t) time;
    struct tm *tms = localtime(&temp);
    timezone = -(tms->tm_gmtoff);

    /* Return                       */
    METHOD_END(ildjitSystem, "Platform.TimeMethods.GetTimeZoneAdjust(long)");
    return (JITINT32) timezone;
}

JITINT64 Platform_TimeMethods_GetCurrentUtcTime (void) {
    struct timeval tv;
    JITINT64 time;

    METHOD_BEGIN(ildjitSystem, "Platform.TimeMethods.GetCurrentUtcTime()");

    /* Fetch the current time	*/
    gettimeofday(&tv, NULL);

    /* Compute the milliseconds	*/
    time = (tv.tv_sec * (JITINT64) 10000000) + (JITINT64) (tv.tv_usec * (JITUINT32) 10);

    /* Return the milliseconds	*/
    METHOD_END(ildjitSystem, "Platform.TimeMethods.GetCurrentUtcTime()");
    return time;
}

JITINT64 Platform_TimeMethods_GetCurrentTime (void) {
    struct timeval tv;
    JITINT64 time;

    METHOD_BEGIN(ildjitSystem, "Platform.TimeMethods.GetCurrentTime()");

    /* Fetch the current time	*/
    gettimeofday(&tv, NULL);

    /* Adjust the time              */
    //tv.secs -= (ILInt64)(ILGetTimeZoneAdjust());

    /* Compute the milliseconds	*/
    time = (tv.tv_sec * (JITINT64) 10000000) + (JITINT64) (tv.tv_usec * (JITUINT32) 10);

    /* Return the milliseconds	*/
    METHOD_END(ildjitSystem, "Platform.TimeMethods.GetCurrentTime()");
    return time;
}

JITINT32 Platform_TimeMethods_GetUpTime (void) {
    JITINT32 milliseconds;
    ProfileTime currentTime;

    METHOD_BEGIN(ildjitSystem, "Platform.TimeMethods.GetUpTime");

    /* Initialize the variables	*/
    milliseconds = 0;

    /* Fetch the current time	*/
    profilerStartTime(&currentTime);

    /* Compute the milliseconds	*/
    milliseconds	= (JITINT32)(profilerGetSeconds(&currentTime) * 1000);

    /* Return the milliseconds	*/
    METHOD_END(ildjitSystem, "Platform.TimeMethods.GetUpTime");
    return milliseconds;
}

static inline JITINT8 * getCultureName () {
    JITINT8 *env;
    JITINT8 name[8];
    JITINT8 *cultureName;

    /* Search inside the LC_ALL environment variable	*/
    env = (JITINT8 *) getenv("LC_ALL");
    if (!env || *env == '\0') {

        /* Search inside the LANG environment variable	*/
        env = (JITINT8 *) getenv("LANG");
        if (!env || *env == '\0') {
            return 0;
        }
    }
    assert(env != NULL);

    /* Validate the first two character of LANG		*/
    if (!isAlpha(env[0]) || !isAlpha(env[1])) {
        return 0;
    }

    /* Convert the LANG value into the ECMA constant	*/
    name[0] = toLower(env[0]);
    name[1] = toLower(env[1]);
    if (env[2] == '\0') {
        cultureName = (JITINT8 *) allocMemory(sizeof(JITINT8) * (strlen((char *) name) + 1));
        memcpy(cultureName, name, strlen((char *) name) + 1);
        return cultureName;
    }
    if (env[2] != '-' && env[2] != '_') {
        return 0;
    }

    /* Validate characters 3 end 4 of LANG			*/
    if (!isAlpha(env[3]) || !isAlpha(env[4])) {
        return 0;
    }
    name[2] = '-';
    name[3] = toUpper(env[3]);
    name[4] = toUpper(env[4]);
    name[5] = '\0';
    cultureName = allocMemory(sizeof(JITINT8) * (strlen((char *) name) + 1));
    memcpy(cultureName, name, strlen((char *) name) + 1);
    return cultureName;
}

static inline JITINT8 toUpper (JITINT8 ch) {
    return ((ch) >= 'a' && (ch) <= 'z') ? (ch) - 'a' + 'A' : (ch);
}

static inline JITINT8 toLower (JITINT8 ch) {
    return ((ch) >= 'A' && (ch) <= 'Z') ? (ch) - 'A' + 'a' : (ch);
}

static inline JITINT16 isAlpha (JITINT8 ch) {
    return ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'));
}

JITINT32 System_BitConverter_FloatToInt32Bits (JITFLOAT32 value) {
    JITINT32 res;

    METHOD_BEGIN(ildjitSystem, "System.BitConverter.FloatToInt32Bits");

    union {
        JITFLOAT32 input;
        JITINT32 output;

    } convert;
    convert.input = value;
    res = convert.output;

    METHOD_END(ildjitSystem, "System.BitConverter.FloatToInt32Bits");
    return res;
}

JITFLOAT32 System_BitConverter_Int32BitsToFloat (JITINT32 value) {
    JITFLOAT32 res;

    METHOD_BEGIN(ildjitSystem, "System.BitConverter.Int32BitsToFloat");

    union {
        JITINT32 input;
        JITFLOAT32 output;

    } convert;
    convert.input = value;
    res = convert.output;

    METHOD_END(ildjitSystem, "System.BitConverter.Int32BitsToFloat");
    return res;
}

JITFLOAT64 System_BitConverter_Int64BitsToDouble (JITINT64 value) {
    JITFLOAT64 res;

    METHOD_BEGIN(ildjitSystem, "System.BitConverter.Int64BitsToDouble");

    union {
        JITINT64 input;
        JITFLOAT64 output;

    } convert;
    convert.input = value;
    res = convert.output;

    METHOD_END(ildjitSystem, "System.BitConverter.Int64BitsToDouble");
    return res;
}

JITBOOLEAN System_BitConverter_GetLittleEndian (void) {
    JITBOOLEAN res;

    METHOD_BEGIN(ildjitSystem, "System.BitConverter.GetLittleEndian");

    union {
        JITUINT8 bytes[4];
        JITUINT32 value;

    } convert;
    convert.value = (JITUINT32) 0x01020304;
    res = (convert.bytes[0] == (JITUINT8) 0x04);

    METHOD_END(ildjitSystem, "System.BitConverter.GetLittleEndian");
    return res;
}

JITINT64 System_BitConverter_DoubleToInt64Bits (JITFLOAT64 value) {
    JITINT64 res;

    METHOD_BEGIN(ildjitSystem, "System.BitConverter.DoubleToInt64Bits");

    union {
        JITFLOAT64 input;
        JITINT64 output;

    } convert;
    convert.input = value;
    res = convert.output;

    METHOD_END(ildjitSystem, "System.BitConverter.DoubleToInt64Bits");
    return res;
}

/*
 * public static Object System.TypedReference.ToObject();
 */
void * System_TypedReference_ToObject (void *typedRef) {
    void **value; /**< pointer to the boxed value */

    METHOD_BEGIN(ildjitSystem, "System.TypedReference.ToObject");

    /* Get the typeDef's data       */
    value = ((ildjitSystem->cliManager).CLR.typedReferenceManager).getValue(typedRef);
    assert(value != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.TypedReference.ToObject");
    return *value;
}
