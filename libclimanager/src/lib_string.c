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
#include <small_unicode.h>
#include <base_symbol.h>
#include <platform_API.h>

// My headers
#include <lib_string.h>
#include <climanager_system.h>
// End

#define CAPACITY_FIELD_AVAILABLE(stringManager) (stringManager.capacityField != NULL)

/* Name of the first character field in the Mono and PNET BCL */
#define MONO_FIRST_CHAR_FIELDNAME (JITINT8 *) "start_char"
#define PNET_FIRST_CHAR_FIELDNAME (JITINT8 *) "firstChar"

typedef struct _UserString {
    t_binary_information *binary;
    JITINT32 row;
    pthread_mutex_t mutex;
    void **object;
} UserString;

extern CLIManager_t *cliManager;

static inline JITINT32 internal_getLength (void *string);
static inline JITINT32 internal_getCapacity (void *string);
static inline void internal_setLength (void *string, JITINT32 length);
static inline void internal_setCapacity (void *string, JITINT32 capacity);
static inline JITINT16 * internal_toLiteral (void *string);
static inline void * internal_newPermanentInstance (JITINT16 *literal, JITINT32 length);
static inline void * internal_newInstance (JITINT16 *literal, JITINT32 length);
static inline void * internal_newBuilder (void *value, JITINT32 length);
static inline TypeDescriptor *internal_fillILStringType ();
static inline void * internal_newInstanceEmpty (JITINT32 length);
static inline void internal_fromUTF8ToUTF16 (JITINT8 *literalUTF8, JITINT16 *literalUTF16, JITINT32 length);
static inline void internal_fromUTF16ToUTF8 (JITINT16 *literalUTF16, JITINT8 *literalUTF8, JITINT32 length);
static inline void internal_ctor_pcii (void *object, JITINT16 * literal, JITINT32 startIndex, JITINT32 length);
static inline void * internal_ConcatStrings (void *str1, void *str2);
static inline JITINT32 internal_CompareStrings (void *strA, JITINT32 indexA, JITINT32 lengthA, void *strB, JITINT32 indexB, JITINT32 lengthB, JITBOOLEAN ignoreCase);
static inline void internal_printLiteral (void *string, JITINT8 *literalUTF8);
static inline void * internal_fetchOrCreateUserStringObject (UserString *userString);
static inline UserString * internal_getFetchOrCreateUserString (t_binary_information *binary, JITUINT32 row);
static inline ir_symbol_t * internal_getUserStringSymbol (t_binary_information *binary, JITUINT32 row);
static inline void * internal_getUserString (t_binary_information *binary, JITUINT32 row);
static inline void internal_check_metadata (void);
static inline void internal_lib_string_initialize (void);
static inline void *internal_trim (void *string, void *whiteSpaces, JITINT32 trimFlag);
static inline JITUINT32 internal_hashFunction (void *string);
static inline JITINT32 internal_hashCompare (void *key1, void *key2);
static inline void *internal_intern (void *string);
static inline void *internal_isInterned (void *string);
static inline JITINT8 *internal_getUTF8copy (void *str);
static inline JITINT32 internal_checkStringLength (void *object, JITINT32 string_length);
static inline void internal_destroyStringManager (t_stringManager *self);
static inline void internal_printdbgString (void *string, char* descrizione, JITINT8 mode);
static inline JITINT32 internal_getFirstCharOffset (void);
static inline ir_value_t userStringResolve (ir_symbol_t *symbol);
static inline void userStringSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void userStringDump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t * userStringDeserialize (void *mem, JITUINT32 memBytes);
static inline void * internal_newInstanceFromUTF8 (JITINT8 *literal, JITINT32 length);

pthread_once_t stringMetadataLock = PTHREAD_ONCE_INIT;

void * CLIMANAGER_StringManager_newInstanceFromUTF8 (JITINT8 *literal, JITINT32 length) {
    return internal_newInstanceFromUTF8(literal, length);
}

JITUINT32 hashUserString (void *element) {
    if (element == NULL) {
        return 0;
    }
    UserString *us = (UserString *) element;
    JITUINT32 seed = combineHash((JITUINT32) us->row, (JITUINT32) us->binary);
    return seed;
}

JITINT32 equalsUserString (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    UserString *us1 = (UserString *) key1;
    UserString *us2 = (UserString *) key2;
    return us1->row == us2->row && us1->binary == us2->binary;
}

void init_stringManager (t_stringManager *stringManager) {

    /* Assertions					*/
    assert(stringManager != NULL);

    /* Initialize the functions			*/
    stringManager->destroy = internal_destroyStringManager;
    stringManager->fillILStringType = internal_fillILStringType;
    stringManager->getLength = internal_getLength;
    stringManager->getCapacity = internal_getCapacity;
    stringManager->setLength = internal_setLength;
    stringManager->setCapacity = internal_setCapacity;
    stringManager->toLiteral = internal_toLiteral;
    stringManager->ctor_pcii = internal_ctor_pcii;
    stringManager->newInstance = internal_newInstance;
    stringManager->newBuilder = internal_newBuilder;
    stringManager->newPermanentInstance = internal_newPermanentInstance;
    stringManager->newInstanceEmpty = internal_newInstanceEmpty;
    stringManager->fromUTF8ToUTF16 = internal_fromUTF8ToUTF16;
    stringManager->fromUTF16ToUTF8 = internal_fromUTF16ToUTF8;
    stringManager->getUTF8copy = internal_getUTF8copy;
    stringManager->concatStrings = internal_ConcatStrings;
    stringManager->compareStrings = internal_CompareStrings;
    stringManager->printLiteral = internal_printLiteral;
    stringManager->getUserString = internal_getUserString;
    stringManager->getUserStringSymbol = internal_getUserStringSymbol;
    stringManager->printdbg = internal_printdbgString;
    stringManager->trim = internal_trim;
    stringManager->toUpper = ILUnicodeStringToUpper;
    stringManager->toLower = ILUnicodeStringToLower;
    stringManager->compareIgnoreCase = ILUnicodeStringCompareIgnoreCase;
    stringManager->compare = ILUnicodeStringCompareNoIgnoreCase;
    stringManager->hashFunction = internal_hashFunction;
    stringManager->hashCompare = internal_hashCompare;
    stringManager->intern = internal_intern;
    stringManager->isInterned = internal_isInterned;
    stringManager->initialize = internal_lib_string_initialize;
    stringManager->getFirstCharOffset = internal_getFirstCharOffset;

    //hash table usata dalle funzioni intern e isInterned
    stringManager->internHashTable = xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, internal_hashFunction, internal_hashCompare);
    stringManager->usObjects = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashUserString, equalsUserString);

    IRSYMBOL_registerSymbolManager(USER_STRING_SYMBOL, userStringResolve, userStringSerialize, userStringDump, userStringDeserialize);

    return ;
}

static inline void internal_printLiteral (void *string, JITINT8 *literalUTF8) {
    JITINT16        *literal;
    JITINT32 length;
    JITINT32 count;

    /* Assertions				*/
    assert(string != NULL);
    assert(literalUTF8 != NULL);
    PDEBUG("LIB_STRING: printLiteral: Start\n");

    /* Fetch the literal of the string	*/
    PDEBUG("LIB_STRING: printLiteral:       String object	= %p\n", string);
    literal = internal_toLiteral(string);
    assert(literal != NULL);
    PDEBUG("LIB_STRING: printLiteral:       Literal		= %p\n", literal);

    /* Fetch the length of the string	*/
    length = internal_getLength( string);
    PDEBUG("LIB_STRING: printLiteral:       Length		= %d\n", length);

    /* Print the literal to copy		*/
    for (count = 0; count < length; count++) {
        PDEBUG("LIB_STRING: printLiteral:       <%p> Abs value[%d]	=%hu\n", &(literal[count]), count, literal[count]);
        literalUTF8[count] = (JITINT8) literal[count];
    }
    literalUTF8[count] = (JITINT8) '\0';
    PDEBUG("LIB_STRING: printLiteral:       Value		= %s\n", literalUTF8);

    /* Return				*/
    PDEBUG("LIB_STRING: printLiteral: Exit\n");
    return;
}

static inline JITINT32 internal_getLength (void *string) {
    JITINT32 lengthFieldOffset;

    /* Initialize the variables		*/
    lengthFieldOffset = 0;

    /* Check if the string is null		*/
    if (string == NULL) {
        return 0;
    }

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    /* Fetch the length offset	*/
    lengthFieldOffset = (cliManager->CLR).stringManager.lengthFieldOffset;

    /* Return the lengths		*/
    return *((JITINT32 *) (string + lengthFieldOffset));
}

static inline JITINT32 internal_getCapacity (void *string) {
    JITINT32 fieldOffset;

    /* Assertions					*/
    assert(string != NULL);

    /* Initialize the variables			*/
    fieldOffset = 0;

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    /* Fetch the length offset	*/
    fieldOffset = (cliManager->CLR).stringManager.capacityFieldOffset;

    /* Return the lengths		*/
    return *((JITINT32 *) (string + fieldOffset));
}

static inline void internal_setLength (void *string, JITINT32 length) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(string != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = (cliManager->CLR).stringManager.lengthFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(string + fieldOffset, &length, sizeof(JITINT32));
    assert(length == internal_getLength( string));

    /* Return							*/
    return;
}

static inline void internal_setCapacity (void *string, JITINT32 capacity) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(string != NULL);

    /* Only PNET BCL strings has the capacity field */
    if (CAPACITY_FIELD_AVAILABLE((cliManager->CLR).stringManager)) {

        /* Check if we have to load the String	*
         * type information			*/
        PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

        /* Fetch the length offset					*/
        fieldOffset = (cliManager->CLR).stringManager.capacityFieldOffset;

        /* Store the length inside the field of the object		*/
        memcpy(string + fieldOffset, &capacity, sizeof(JITINT32));
        assert(capacity == internal_getCapacity( string));
    }

    /* Return							*/
    return;
}

/* Get a pointer to the string raw representation */
static inline JITINT16* internal_toLiteral (void* string) {
    return string + internal_getFirstCharOffset();
}

static inline void * internal_newInstanceFromUTF8 (JITINT8 *literal, JITINT32 length) {
    void            *string;
    JITINT16        *literalUTF16;

    /* Assertions				*/
    assert(literal != NULL);

    /* Initialize the variables		*/
    string = NULL;

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    string = internal_newInstanceEmpty(length);
    assert(string != NULL);

    /* Convert the literal to UTF16		*/
    literalUTF16 = allocMemory(sizeof(JITINT16) * length);
    internal_fromUTF8ToUTF16(literal, literalUTF16, length);

    /* Initialize the System.String		*/
    internal_ctor_pcii(string, literalUTF16, 0, length);
    freeMemory(literalUTF16);

    /* Return the new instance of           *
     * System.String			*/
    return string;
}

static inline void internal_fromUTF16ToUTF8 (JITINT16 *literalUTF16, JITINT8 *literalUTF8, JITINT32 length) {
    JITINT32 count;

    /* Assertions			*/
    assert(literalUTF8 != NULL);
    assert(literalUTF16 != NULL);
    PDEBUG("LIB_STRING: fromUTF16ToUTF8: Start\n");
    PDEBUG("LIB_STRING: fromUTF16ToUTF8:    Length	= %d\n", length);

    for (count = 0; count < length; count++) {
        literalUTF8[count] = (literalUTF16[count] & 0xFF);
        PDEBUG("LIB_STRING: fromUTF16ToUTF8:    Char %d		UTF16 = 0x%X	UTF8 = %c\n", count, literalUTF16[count], literalUTF8[count]);
    }
    literalUTF8[length] = '\0';
    PDEBUG("LIB_STRING: fromUTF16ToUTF8:    Literal UTF8	= %s\n", literalUTF8);

    /* Return			*/
    PDEBUG("LIB_STRING: fromUTF16ToUTF8: Exit\n");
    return;
}

static inline void internal_fromUTF8ToUTF16 (JITINT8 *literalUTF8, JITINT16 *literalUTF16, JITINT32 length) {
    JITINT32 count;

    /* Assertions			*/
    assert(literalUTF8 != NULL);
    assert(literalUTF16 != NULL);

    for (count = 0; count < length; count++) {
        literalUTF16[count] = literalUTF8[count];
    }
}

static inline void * internal_newInstanceEmpty (JITINT32 length) {
    void            *string;
    JITINT32 roundLen;

    /* Initialize the variables		*/
    string = NULL;

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    /* Round length to a multiple of 8 */
    roundLen = (length + 7) & ~7;

    /* Make the System.String		*/
    TypeDescriptor *type = (cliManager->CLR).stringManager.StringType;
    string = cliManager->gc->allocObject(type, sizeof(JITINT16) * roundLen);
    if (string == NULL) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        abort();
    }
    assert(string != NULL);
    assert(internal_checkStringLength(string, roundLen));

    /* Initialize the System.String		*/
    internal_setCapacity( string, roundLen);
    internal_setLength( string, length);

    /* Return the new instance of           *
     * System.String			*/
    return string;
}

static inline void * internal_newPermanentInstance (JITINT16 *literal, JITINT32 length) {
    void            *string;

    /* Assertions				*/
    assert(literal != NULL);

    /* Initialize the variables		*/
    string = NULL;

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    /* Make the System.String		*/
    TypeDescriptor *type = (cliManager->CLR).stringManager.StringType;
    string = cliManager->gc->allocPermanentObject(type, sizeof(JITINT16) * length);
    if (string == NULL) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        abort();
    }
    assert(string != NULL);

    /* Initialize the System.String		*/
    internal_ctor_pcii( string, literal, 0, length);

    /* Return the new instance of           *
     * System.String			*/
    return string;
}

static inline void * internal_newInstance (JITINT16 *literal, JITINT32 length) {
    void            *string;

    /* Assertions				*/
    assert(literal != NULL);

    /* Initialize the variables		*/
    string = NULL;

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    /* Make the System.String		*/
    string = internal_newInstanceEmpty( length);
    assert(string != NULL);

    /* Initialize the System.String		*/
    internal_ctor_pcii( string, literal, 0, length);

    /* Return the new instance of           *
     * System.String			*/
    return string;
}

static inline void * internal_newBuilder (void *value, JITINT32 length) {
    JITINT32 roundLen;
    JITINT32 valueLength;
    void            *newString;
    JITINT16        *valueLiteral;
    JITINT16        *stringLiteral;

    /* Check the overflow		*/
    if (length == -1) {
        roundLen = internal_getLength( value);
    } else {
        roundLen = length;
    }

    /* Compute the roundLen		*/
    roundLen = (roundLen + 7) & ~7;          /* Round to a multiple of 8 */

    /* Make the System.String		*/
    TypeDescriptor *type = (cliManager->CLR).stringManager.StringType;
    newString = cliManager->gc->allocObject(type, sizeof(JITINT16) * roundLen);
    if (newString == NULL) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        abort();
    }
    assert(newString != NULL);

    /* Initialize the System.String		*/
    internal_setCapacity( newString, roundLen);
    if (value != NULL) {
        valueLength = internal_getLength( value);
        assert(valueLength >= 0);
        valueLiteral = internal_toLiteral(value);
        assert(valueLiteral != NULL);
        stringLiteral = internal_toLiteral(newString);
        assert(valueLiteral != NULL);

        if (valueLength <= roundLen) {
            memcpy(stringLiteral, valueLiteral, valueLength * sizeof(JITINT16));
            internal_setLength( newString, valueLength);
        } else {
            memcpy(stringLiteral, valueLiteral, roundLen * sizeof(JITINT16));
            internal_setLength( newString, roundLen);
        }
    } else {
        internal_setLength( newString, 0);
    }

    /* Check the string		*/
    assert(newString != NULL);
    assert(internal_checkStringLength(newString, roundLen));

    return newString;
}

static inline void internal_ctor_pcii (void *object, JITINT16 * literal, JITINT32 startIndex, JITINT32 length) {
    JITINT32 firstCharOffset = 0;

    /* Assertions			*/
    assert(object != NULL);

    firstCharOffset = internal_getFirstCharOffset();

    /* Store the literal inside the overSize of the String object	*/
    if (length > 0) {
        assert(literal != NULL);
        memcpy(object + firstCharOffset, literal + startIndex, sizeof(JITINT16) * length);
    }

    /* Store the capacity of the string				*/
    internal_setCapacity( object, length);

    /* Store the length of the string				*/
    internal_setLength(object, length);

    /* Exit								*/
    return;
}

static inline void * internal_ConcatStrings (void *str1, void *str2) {
    JITUINT32 length1;
    JITUINT32 length2;
    JITUINT32 newLength;
    JITINT16                *string_literal_1;
    JITINT16                *string_literal_2;
    JITINT16                *newLiteral;
    void                    *newObject;

    /* Initialize the variables		*/
    string_literal_1 = NULL;
    string_literal_2 = NULL;
    newObject = NULL;
    length1 = 0;
    length2 = 0;

    /* Fetch the literals			*/
    if (str1) {
        string_literal_1 = internal_toLiteral(str1);
        assert(string_literal_1 != NULL);
        length1 = internal_getLength( str1);
        assert(length1 >= 0);
    }
    if (str2) {
        string_literal_2 = internal_toLiteral(str2);
        assert(string_literal_2 != NULL);
        length2 = internal_getLength( str2);
        assert(length2 >= 0);
    }

    /* Compute the length of the    *
     * new string			*/
    newLength = length1 + length2;

    /* Make a new System.String     *
     * object			*/
    newObject = internal_newInstanceEmpty( newLength);
    assert(newObject != NULL);

    newLiteral = internal_toLiteral(newObject);
    assert(newLiteral != NULL);

    if (str1) {
        memcpy(newLiteral, string_literal_1, sizeof(JITINT16) * length1);
    }
    if (str2) {
        memcpy(newLiteral + length1, string_literal_2, sizeof(JITINT16) * length2);
    }

    /* Return the new string	*/
    return newObject;
}


static inline JITINT32 internal_CompareStrings (void *strA, JITINT32 indexA, JITINT32 lengthA, void *strB, JITINT32 indexB, JITINT32 lengthB, JITBOOLEAN ignoreCase) {
    JITINT32 result;
    JITINT32 cmp;
    JITINT16        *bufA;
    JITINT16        *bufB;

    /* Handle the easy cases first */
    if (!strA) {
        if (!strB) {
            result = 0;
        } else {
            result = -1;
        }
    } else if (!strB) {
        result = 1;
    } else {
        bufA = internal_toLiteral(strA) + indexA;
        assert(bufA != NULL);
        bufB = internal_toLiteral(strB) + indexB;
        assert(bufB != NULL);

        /* Compare the two strings */
        if (lengthA >= lengthB) {
            if (ignoreCase) {
                cmp = ILUnicodeStringCompareIgnoreCase(bufA, bufB, (JITINT32) lengthB);
            } else {
                cmp = ILUnicodeStringCompareNoIgnoreCase(bufA, bufB, (JITINT32) lengthB);
            }

            if (cmp != 0) {
                result = cmp;
            } else if (lengthA > lengthB) {
                result = 1;
            } else {
                result = 0;
            }
        } else {
            if (ignoreCase) {
                cmp = ILUnicodeStringCompareIgnoreCase(bufA, bufB, lengthA);
            } else {
                cmp = ILUnicodeStringCompareNoIgnoreCase(bufA, bufB, lengthA);
            }

            if (cmp != 0) {
                result = cmp;
            } else {
                result = -1;
            }
        }
    }

    return result;
}

static inline TypeDescriptor *internal_fillILStringType () {

    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    /* Return				*/
    return (cliManager->CLR).stringManager.StringType;
}


static inline void * internal_fetchOrCreateUserStringObject (UserString *userString) {
    PLATFORM_lockMutex(&(userString->mutex));

    /* Check if there exist the US  object	*/
    if (*(userString->object)==NULL) {
        t_row_us_stream usRow;
        JITINT16        *stringUTF16;
        JITUINT32 bytesString;

        /* Load user string row from stream */
        get_us_row(&((userString->binary->metadata).streams_metadata.us_stream), userString->row, &usRow);
        assert(usRow.string != NULL);

        /* Build the terminated string	*/
        stringUTF16 = (JITINT16 *) allocMemory(usRow.bytesLength + 2);
        memcpy(stringUTF16, usRow.string, usRow.bytesLength);
        ((JITUINT8 *) stringUTF16)[usRow.bytesLength] = '\0';
        ((JITUINT8 *) stringUTF16)[usRow.bytesLength + 1] = '\0';
        assert(stringUTF16 != NULL);

        bytesString = usRow.bytesLength;
        assert(bytesString >= 0);

        /* Allocate the string		*/
        *(userString->object) = internal_newPermanentInstance( stringUTF16, (bytesString / 2));
        assert(*(userString->object) != NULL);

        /* We don't need the terminated string anymore	*/
        freeMemory(stringUTF16);

        /* Mark object accessible by Intern e IsInterned functions */
        internal_intern(*(userString->object));
    }

    PLATFORM_unlockMutex(&(userString->mutex));
    return *(userString->object);
}

static inline UserString * internal_getFetchOrCreateUserString (t_binary_information *binary, JITUINT32 row) {
    UserString us;

    us.binary = binary;
    us.row = row;

    xanHashTable_lock((cliManager->CLR).stringManager.usObjects);
    UserString *found = (UserString *) xanHashTable_lookup((cliManager->CLR).stringManager.usObjects, (void *) &us);
    if (found == NULL) {
        found = (UserString *) sharedAllocFunction(sizeof(UserString));
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->binary = binary;
        found->row = row;
        found->object = MEMORY_obtainPrivateAddress();
        assert(*(found->object) == NULL);

        xanHashTable_insert((cliManager->CLR).stringManager.usObjects, (void *) found, (void *)found);
    }
    xanHashTable_unlock((cliManager->CLR).stringManager.usObjects);

    return found;
}

static inline void * internal_getUserString (t_binary_information *binary, JITUINT32 row) {
    UserString *us = internal_getFetchOrCreateUserString(binary, row);

    return internal_fetchOrCreateUserStringObject(us);
}

static inline ir_value_t userStringResolve (ir_symbol_t *symbol) {
    ir_value_t value;

    value.v = (JITNUINT) internal_fetchOrCreateUserStringObject((UserString *) symbol->data);
    return value;
}

static inline void userStringSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    JITUINT32	bytesWritten;

    /* Fetch the string.
     */
    UserString *userString = (UserString *) symbol->data;

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*memBytesAllocated)	+= STRLEN(userString->binary->name) + 2;
    (*mem)			= allocFunction(*memBytesAllocated);
    bytesWritten		= 0;

    /* Serialize the string.
     */
    bytesWritten    += ILDJIT_writeStringInMemory(*mem, bytesWritten, *memBytesAllocated, userString->binary->name, JITTRUE);
    bytesWritten    += ILDJIT_writeIntegerValueInMemory(*mem, bytesWritten, *memBytesAllocated, userString->row, JITFALSE);
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void userStringDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    fprintf(fileToWrite, "Pointer to User Defined String");
}

static inline ir_symbol_t * internal_getUserStringSymbol (t_binary_information *binary, JITUINT32 row) {
    UserString *us = internal_getFetchOrCreateUserString(binary, row);

    return IRSYMBOL_createSymbol(USER_STRING_SYMBOL, (void *) us);
}

static inline ir_symbol_t * userStringDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT8 *binaryName;
    t_binary_information *binary;
    JITINT32 	row;
    JITUINT32	bytesRead;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Initialize the variables.
     */
    binary = NULL;

    /* Read the name of the binary.
     */
    bytesRead	= ILDJIT_readStringFromMemory(mem, 0, &binaryName);

    /* Fetch the binary.
     */
    XanList *binaries = cliManager->metadataManager->binaries;
    XanListItem *item = xanList_first(binaries);
    while (item != NULL) {
        binary = (t_binary_information *) item->data;
        if (STRCMP(binary->name, binaryName)==0) {
            break;
        }
        item = item->next;
    }

    /* Read the row number.
     */
    bytesRead	+= ILDJIT_readIntegerValueFromMemory(mem, bytesRead, &row);

    /* Free the memory.
     */
    freeFunction(binaryName);

    return internal_getUserStringSymbol(binary, row);
}

static inline void internal_check_metadata (void) {
    JITBOOLEAN 	monoBCLIsLoaded;
    JITINT8         *firstCharFieldName;

    /* Check if we have to load the metadata	*/
    if ((cliManager->CLR).stringManager.StringType == NULL) {

        /* Fetch the System.String type			*/
        (cliManager->CLR).stringManager.StringType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "String");

        /* Fetch the length field ID			*/
        (cliManager->CLR).stringManager.lengthField = (cliManager->CLR).stringManager.StringType->getFieldFromName((cliManager->CLR).stringManager.StringType, (JITINT8 *) "length");

        /* Fetch the offset				*/
        (cliManager->CLR).stringManager.lengthFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).stringManager.lengthField);

        /* Fetch the capacity field ID			*/
        (cliManager->CLR).stringManager.capacityField = (cliManager->CLR).stringManager.StringType->getFieldFromName((cliManager->CLR).stringManager.StringType, (JITINT8 *) "capacity");

        /* The capacity field is present only in the PNET BCL */
        if ((cliManager->CLR).stringManager.capacityField != NULL) {
            (cliManager->CLR).stringManager.capacityFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).stringManager.capacityField);
            monoBCLIsLoaded = JITFALSE;
        } else {
            monoBCLIsLoaded = JITTRUE;
        }

        /* Fetch the first character field ID */
        if (monoBCLIsLoaded) {
            firstCharFieldName = MONO_FIRST_CHAR_FIELDNAME;
        } else {
            firstCharFieldName = PNET_FIRST_CHAR_FIELDNAME;
        }
        (cliManager->CLR).stringManager.firstCharField = (cliManager->CLR).stringManager.StringType->getFieldFromName((cliManager->CLR).stringManager.StringType, firstCharFieldName);

        (cliManager->CLR).stringManager.firstCharFieldOffset = cliManager->gc->fetchFieldOffset((cliManager->CLR).stringManager.firstCharField);
    }
    assert((cliManager->CLR).stringManager.lengthField != NULL);
    assert((cliManager->CLR).stringManager.StringType != NULL);

    /* Return					*/
    return;
}

static inline JITBOOLEAN internal_isWhiteSpace (JITINT16 c, JITINT16 *w, JITINT32 w_len) {
    JITINT32 i;
    JITBOOLEAN ret = 0;

    if (w != NULL) {
        for (i = 0; i < w_len && ret == 0; i++) {
            if (c == w[i]) {
                return JITTRUE;
            }
        }
    }

    return JITFALSE;
}

static inline JITINT32 internal_firstNotWhite (JITINT16 *s, JITINT32 s_len, JITINT16 *w, JITINT32 w_len) {
    JITINT32 i;

    for (i = 0; i < s_len && internal_isWhiteSpace(s[i], w, w_len); i++) {
        ;
    }

    return i;
}

static inline JITINT32 internal_lastNotWhite (JITINT16 *s, JITINT32 s_len, JITINT16 *w, JITINT32 w_len) {
    JITINT32 i;

    for (i = s_len - 1; i >= 0 && internal_isWhiteSpace(s[i], w, w_len); i--) {
        ;
    }

    return i;
}

static inline void *internal_trim (void *string, void *whiteSpaces, JITINT32 trimFlag) {
    JITINT16        *s;
    JITINT32 s_len;
    JITINT16        *w;
    JITINT32 w_len;
    JITINT16        *ret = NULL;
    JITINT32 ret_len = 0;
    void            *obj;
    JITINT32 TrimFlag_Front = 1;
    JITINT32 TrimFlag_End = 2;
    JITINT32 i, start, end, first, last;

    /* Initialize the variables			*/
    i = 0;
    start = 0;
    end = 0;
    first = 0;
    last = 0;

    //converte l'oggetto stringa in un vettore di s_len caratteri UTF16
    s = internal_toLiteral(string);
    assert(s != NULL);
    s_len = internal_getLength(string);
    assert(s_len >= 0);

    //w è un oggetto vettore di caratteri UTF16 (elenco dei caratteri da tagliare)
    w = (cliManager->CLR).arrayManager.getArrayElement(whiteSpaces, 0);
    assert(w != NULL);
    w_len = (cliManager->CLR).arrayManager.getArrayLength(whiteSpaces, 0);
    assert(w_len >= 0);

    first = internal_firstNotWhite(s, s_len, w, w_len);
    last = internal_lastNotWhite(s, s_len, w, w_len);

    if (first != s_len - 1 && last != -1) {
        if (trimFlag == TrimFlag_Front) { //trim start
            start = first;
            end = s_len - 1;
        } else if (trimFlag == TrimFlag_End) { //trim end
            start = 0;
            end = last;
        } else if (trimFlag == (TrimFlag_Front | TrimFlag_End)) { //trim start and end
            start = first;
            end = last;
        }

        ret_len = end - start + 1;
    }

    ret = (JITINT16*) allocMemory((ret_len + 1) * sizeof(JITINT16));
    for (i = 0; i < ret_len; i++) {
        ret[i] = s[start + i];
    }
    ret[ret_len] = 0;

    obj = internal_newInstance(ret, ret_len);

    freeMemory(ret);
    return obj;
}

static inline JITUINT32 internal_hashFunction (void *string) {
    JITINT32 hash = 0;
    JITINT16        *buf;
    JITINT32 buf_len;
    JITINT32 i;

    buf_len = internal_getLength(string);
    assert(buf_len >= 0);
    buf = internal_toLiteral(string);
    assert(buf != NULL);

    for (i = 0; i < buf_len; i++) {
        hash = (hash << 5) + hash + (JITINT32) (buf[i]);
    }

    return hash;
}

static inline JITINT32 internal_hashCompare (void *key1, void *key2) {
    JITINT32 length1 = 0;
    JITINT32 length2 = 0;

    if (key1) {
        length1 = internal_getLength(key1);
    }
    assert(length1 >= 0);
    if (key2) {
        length2 = internal_getLength(key2);
    }
    assert(length2 >= 0);

    if (internal_CompareStrings(key1, 0, length1, key2, 0, length2, 0) == 0) {
        return 1;
    }

    /* Return			*/
    return 0;
}

static inline void * internal_intern (void *string) {
    XanHashTable    *table;

    /* Assertions					*/
    assert(string != NULL);

    table = (cliManager->CLR).stringManager.internHashTable;
    assert(table != NULL);

    xanHashTable_lock(table);
    if (!xanHashTable_lookup(table, string)) {
        xanHashTable_insert(table, string, string);
    }
    assert(xanHashTable_lookup(table, string) != NULL);
    xanHashTable_unlock(table);

    /* Return					*/
    return string;
}

static inline void *internal_isInterned (void *string) {
    void            *ret;
    XanHashTable    *table;

    table = (cliManager->CLR).stringManager.internHashTable;
    assert(table != NULL);

    xanHashTable_lock(table);
    ret = xanHashTable_lookup(table, string);
    xanHashTable_unlock(table);

    return ret;
}

/*AGGIUNTE PER IL DEBUG*/
static inline void internal_printdbgString (void *string, char* descrizione, JITINT8 mode) {
    JITINT8         *str8;
    JITINT16        *str16;
    JITINT32 len;
    JITINT32 i;


    if (string == NULL) {
        fprintf(stderr, "\n %s = NULL", descrizione);
    } else {
        len = internal_getLength(string);

        if (len < 0 ) {
            fprintf(stderr, "\n %s.Length < 0", descrizione);
        } else if (len == 0) {
            fprintf(stderr, "\n  %s.Length = 0", descrizione);
        } else {
            str16 = internal_toLiteral(string);

            if (str16 == NULL) {
                fprintf(stderr, "\n %s.toLiteral = NULL", descrizione);
            } else {
                str8 = (JITINT8*) str16;

                if (!mode) {
                    fprintf(stderr, "\n (%p) %s = '", string, descrizione);
                }

                for (i = 0; i < 2 * len; i++) {
                    if (mode) {
                        fprintf(stderr, "\n %s[%d] = '%c' (%d)", descrizione, i, str8[i], str8[i]);
                    } else {
                        fprintf(stderr, "%c", str8[i]);
                    }
                }

                if (!mode) {
                    fprintf(stderr, "'");
                }
            }
        }
    }
}

static inline JITINT8 *internal_getUTF8copy (void *str) {
    JITINT32 len;
    JITINT16        *literal16;
    JITINT8         *literal8;

    if (str == NULL) {
        return NULL;
    }

    /*estrae caratteri e lunghezza dell'oggetto stringa pattern*/
    len = internal_getLength(str);
    assert(len >= 0);
    literal16 = internal_toLiteral(str);
    assert(literal16 != NULL);

    literal8 = (JITINT8*) allocMemory(sizeof(JITINT8) * (len + 1));
    if (!literal8) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
        return NULL;
    }

    internal_fromUTF16ToUTF8(literal16, literal8, len);
    literal8[len] = '\0';

    return literal8;
}

static inline JITINT32 internal_checkStringLength (void *object, JITINT32 string_length) {
    JITINT32 objSize;
    JITINT32 typeSize;
    JITINT32 overSize;
    JITINT32 strSize;

    assert(object != NULL);

    objSize = cliManager->gc->getSize(object);
    assert(objSize >= 0);
    typeSize = cliManager->gc->getTypeSize(object);
    assert(typeSize >= 0);
    overSize = objSize - HEADER_FIXED_SIZE - typeSize;

    strSize = string_length * sizeof(JITINT16);
    if (overSize < strSize) {
        return JITFALSE;
    }
    return JITTRUE;
}

static inline void internal_lib_string_initialize (void) {

    /* Initialize the module.
     */
    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    return;
}

static inline void internal_destroyStringManager (t_stringManager *self) {
    XanHashTableItem	*item;

    /* Destroy user strings.
     */
    item	= xanHashTable_first((cliManager->CLR).stringManager.usObjects);
    while (item != NULL) {
        UserString *us;
        us	= item->element;
        freeFunction(us->object);
        freeFunction(us);
        item	= xanHashTable_next((cliManager->CLR).stringManager.usObjects, item);
    }

    /* Destroy the used hash tables.
     */
    xanHashTable_destroyTable((cliManager->CLR).stringManager.internHashTable);
    xanHashTable_destroyTable((cliManager->CLR).stringManager.usObjects);

    return ;
}

/* Get the offset of the first character of the string */
static inline JITINT32 internal_getFirstCharOffset (void) {

    PLATFORM_pthread_once(&stringMetadataLock, internal_check_metadata);

    return (cliManager->CLR).stringManager.firstCharFieldOffset;

}
