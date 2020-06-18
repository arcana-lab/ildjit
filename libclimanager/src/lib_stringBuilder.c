/*
 * Copyright (C) 2006 - 2010 Campanoni Simone
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
#include <assert.h>
#include <platform_API.h>

// My headers
#include <lib_stringBuilder.h>
#include <climanager_system.h>
// End

#define MinCapacity             16
#define ResizeRound             31
#define ResizeRoundBig          1023

extern CLIManager_t *cliManager;

static inline JITINT32 getStringBuilderMaxCapacity (void *stringBuilder);
static inline void * internal_getStringFromStringBuilder (void *stringBuilder);
static inline void setStringFromStringBuilder (void *stringBuilder, void *string);
static inline void * internal_Append (void *stringBuilder, JITINT16 *literal, JITINT32 stringLength);
static inline JITINT32 getStringBuilderNewCapacity (JITINT32 length, JITINT32 oldCapacity);
static inline JITINT8 getStringBuilderNeedsCopy (void *stringBuilder);
static inline void setStringBuilderNeedsCopy (void *stringBuilder, JITINT8 value);
static inline void *internal_Insert (void *stringBuilder, JITINT32 index, JITINT32 length);
static inline void internal_lib_stringBuilder_initialize (void);
static inline void internal_lib_stringBuilder_check_metadata (void);
static inline void internal_printdbgStringBuilder (void *string, char* descrizione, JITINT8 mode);

pthread_once_t stringBuilderMetadataLock = PTHREAD_ONCE_INIT;

void init_stringBuilderManager (t_stringBuilderManager *stringBuilderManager) {

    /* Assertions				*/
    assert(stringBuilderManager != NULL);

    /* Init the functions pointer		*/
    stringBuilderManager->getString = internal_getStringFromStringBuilder;
    stringBuilderManager->setString = setStringFromStringBuilder;
    stringBuilderManager->getMaxCapacity = getStringBuilderMaxCapacity;
    stringBuilderManager->append = internal_Append;
    stringBuilderManager->getNewCapacity = getStringBuilderNewCapacity;
    stringBuilderManager->setNeedsCopy = setStringBuilderNeedsCopy;
    stringBuilderManager->getNeedsCopy = getStringBuilderNeedsCopy;
    stringBuilderManager->insert = internal_Insert;
    stringBuilderManager->printdbg = internal_printdbgStringBuilder;
    stringBuilderManager->initialize = internal_lib_stringBuilder_initialize;

    /* Return				*/
    return;
}

static inline void setStringFromStringBuilder (void *stringBuilder, void *string) {
    JITINT32 fieldOffset;

    /* Assertions					*/
    assert(stringBuilder != NULL);

    /* Initialize the variables			*/
    fieldOffset = 0;

    /* Check the metadata				*/
    PLATFORM_pthread_once(&stringBuilderMetadataLock, internal_lib_stringBuilder_check_metadata);

    /* Fetch the length offset	*/
    fieldOffset = ((cliManager->CLR).stringBuilderManager).buildStringOffset;
    assert(fieldOffset >= 0);

    /* Store the string inside the  *
     * string builder		*/
    (*((void **) (((JITINT8 *) stringBuilder) + fieldOffset))) = string;

    /* Return                       */
    return;
}

static inline void * internal_getStringFromStringBuilder (void *stringBuilder) {
    JITINT32 fieldOffset;

    /* Assertions					*/
    assert(stringBuilder != NULL);

    /* Initialize the variables			*/
    fieldOffset = 0;

    /* Check the metadata				*/
    PLATFORM_pthread_once(&stringBuilderMetadataLock, internal_lib_stringBuilder_check_metadata);

    /* Fetch the length offset	*/
    fieldOffset = ((cliManager->CLR).stringBuilderManager).buildStringOffset;
    assert(fieldOffset >= 0);

    return *((void **) (((JITINT8 *) stringBuilder) + fieldOffset));
}

static inline JITINT32 getStringBuilderMaxCapacity (void *stringBuilder) {
    JITINT32 fieldOffset;

    /* Assertions					*/
    assert(stringBuilder != NULL);

    /* Initialize the variables			*/
    fieldOffset = 0;

    /* Check the metadata				*/
    PLATFORM_pthread_once(&stringBuilderMetadataLock, internal_lib_stringBuilder_check_metadata);

    /* Fetch the offset		*/
    fieldOffset = ((cliManager->CLR).stringBuilderManager).maxCapacityOffset;
    assert(fieldOffset >= 0);

    /* Return the lengths		*/
    return *((JITINT32 *) (((JITINT8 *) stringBuilder) + fieldOffset));
}

static inline void * internal_Append (void *stringBuilder, JITINT16 *literal, JITINT32 stringLength) {
    void                    *string;
    JITINT32 oldStringLength;
    JITINT32 newStringLength;
    JITINT32 stringCapacity;
    JITINT32 stringBuilderMaxCapacity;
    JITINT32 newCapacity;
    JITINT8 needscopy;
    JITINT16                *stringLiteral;

    /* Assertions							*/
    assert(stringBuilder != NULL);
    assert(literal != NULL);
    assert(stringLength > 0);

    /* Fetch the string from the stringBuilder			*/
    string = internal_getStringFromStringBuilder(stringBuilder);
    assert(string != NULL);

    /* Compute the new string length				*/
    oldStringLength = (cliManager->CLR).stringManager.getLength(string);
    newStringLength = oldStringLength + stringLength;

    /* Fetch the capacity of the string stored inside the string    *
     * builder							*/
    stringCapacity = (cliManager->CLR).stringManager.getCapacity(string);

    /* Check if we have space inside the string builder to append   *
     * the string							*/
    if (newStringLength > stringCapacity) {
        /* Fetch the max capacity of the string builder		*/
        stringBuilderMaxCapacity = getStringBuilderMaxCapacity(stringBuilder);

        /* Check if we have to throw the exception for the      *
         * arguments						*/
        if (newStringLength > stringBuilderMaxCapacity) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
            return NULL;
        }

        /* Compute the new capacity of the new string builder	*/
        newCapacity = getStringBuilderNewCapacity(newStringLength, stringCapacity);
        if (newCapacity > stringBuilderMaxCapacity) {
            newCapacity = stringBuilderMaxCapacity;
        }


        /* Make the new string builder				*/
        string = (cliManager->CLR).stringManager.newBuilder(string, newCapacity);
        assert(string != NULL);

        /* Store the new string builder inside the _this string	*
         * builder as it is a normal string			*/
        setStringFromStringBuilder(stringBuilder, string);
        setStringBuilderNeedsCopy(stringBuilder, 0);
    } else {
        needscopy = getStringBuilderNeedsCopy(stringBuilder);

        if (needscopy) {
            string = (cliManager->CLR).stringManager.newBuilder(string, stringCapacity);
            assert(string != NULL);

            setStringFromStringBuilder(stringBuilder, string);
            setStringBuilderNeedsCopy(stringBuilder, 0);
        }
    }

    /* Fetch the literal						*/
    stringLiteral = (cliManager->CLR).stringManager.toLiteral(string);
    assert(stringLiteral != NULL);
    assert(stringLiteral != literal);

    /* Append the string						*/
    memcpy(stringLiteral + oldStringLength, literal, sizeof(JITINT16) * stringLength);

    /* Set the new string length					*/
    (cliManager->CLR).stringManager.setLength(string, newStringLength);

    /* Return the string builder					*/
    return stringBuilder;
}

/*
 * calculate the capacity for the new buildString
 *
 */
static inline JITINT32 getStringBuilderNewCapacity (JITINT32 length, JITINT32 oldCapacity) {

    /* for stringbuilder >= 2048 chars add 1024 chars */
    if (length > 2047) {
        return (length + ResizeRoundBig) & ~ResizeRoundBig;
    } else {
        if (oldCapacity == 0) {
            oldCapacity = 1;
        }
        while (oldCapacity < length) {
            oldCapacity *= 2;
        }
        return oldCapacity;
    }
}

static inline void setStringBuilderNeedsCopy (void *stringBuilder, JITINT8 value) {
    JITINT32 fieldOffset;

    /* Assertions					*/
    assert(stringBuilder != NULL);

    /* Initialize the variables			*/
    fieldOffset = 0;

    /* Check the metadata				*/
    PLATFORM_pthread_once(&stringBuilderMetadataLock, internal_lib_stringBuilder_check_metadata);

    /* Fetch the offset		*/
    fieldOffset = ((cliManager->CLR).stringBuilderManager).needsCopyOffset;
    assert(fieldOffset >= 0);

    /* Store the string inside the  *
     * string builder		*/
    (*(((JITINT8*) stringBuilder) + fieldOffset)) = value;

    /* Return                       */
    return;
}

static inline JITINT8 getStringBuilderNeedsCopy (void *stringBuilder) {
    JITINT32 fieldOffset;

    /* Assertions					*/
    assert(stringBuilder != NULL);

    /* Initialize the variables			*/
    fieldOffset = 0;

    /* Check the metadata				*/
    PLATFORM_pthread_once(&stringBuilderMetadataLock, internal_lib_stringBuilder_check_metadata);

    /* Fetch the length offset	*/
    fieldOffset = ((cliManager->CLR).stringBuilderManager).needsCopyOffset;
    assert(fieldOffset >= 0);

    /* Return       */
    return *(((JITINT8*) stringBuilder) + fieldOffset);
}


static inline void *internal_Insert (void *stringBuilder, JITINT32 index, JITINT32 length) {
    JITINT32 newLength;
    JITINT32 capacity;
    JITINT32 newCapacity;
    JITINT32 maxCapacity;
    JITINT32 sbStringLength;
    void            *sbString;
    JITINT16        *sbStringLiteral;
    JITINT16        *strLiteral;
    void            *str;
    JITINT32 needscopy;

    /* Assertions		*/
    assert(stringBuilder != NULL);

    /* Fetch the string from the stringBuilder */
    sbString = internal_getStringFromStringBuilder(stringBuilder);
    assert(sbString != NULL);

    /* lunghezza della stringa*/
    sbStringLength = (cliManager->CLR).stringManager.getLength(sbString);
    assert(sbStringLength >= 0);

    /* capacità della stringa*/
    capacity = (cliManager->CLR).stringManager.getCapacity(sbString);
    assert(capacity >= 0);

    /* capacità massima dello stringbuilder*/
    maxCapacity = getStringBuilderMaxCapacity(stringBuilder);
    assert(maxCapacity >= 0);

    if (index < 0 || index > sbStringLength) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
        return NULL;
    }

    /*calcola la lunghezza della nuova stringa*/
    newLength = sbStringLength + length;


    if (newLength > capacity) { /*caso in cui la nuova stringa è maggiore della capacità dello stringbuilder*/
        if (newLength > maxCapacity) {
            cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
            return NULL;
        }

        /*capacità della nuova stringa da inserire nel nuovo stringbuilder*/
        newCapacity = getStringBuilderNewCapacity(newLength, capacity);
        if (newCapacity > maxCapacity) {
            newCapacity = maxCapacity;
        }

        /*nuova stringa*/
        str = (cliManager->CLR).stringManager.newBuilder(sbString, newCapacity);
        assert(str != NULL);

        /*estrazione caratteri della nuova stringa*/
        strLiteral = (cliManager->CLR).stringManager.toLiteral(str);
        assert(strLiteral != NULL);

        /*estrazione caratteri della stringa del vecchio stringbuilder*/
        sbStringLiteral = (cliManager->CLR).stringManager.toLiteral(sbString);
        assert(sbStringLiteral != NULL);

        /*copia dei caratteri della vecchia stringa nella nuova, lasciando uno spazio di lunghezza length dopo index*/
        if (index > 0) {
            memcpy(strLiteral, sbStringLiteral, sizeof(JITINT16) * (index - 1));
        }
        memcpy(strLiteral + index + length, sbStringLiteral + index, sizeof(JITINT16) * (sbStringLength - index));

        /*imposta lunghezza della nuova stringa*/
        (cliManager->CLR).stringManager.setLength(str, newLength);

        /*inserisce la nuova stringa nel vecchio stringbuilder*/
        setStringFromStringBuilder(stringBuilder, str);

    } else {
        /*caso in cui la nuova stringa è minore della capacità dello stringbuilder*/
        needscopy = getStringBuilderNeedsCopy(stringBuilder);

        if (needscopy) { /*è necessario creare un nuovo stringbuilder*/
            /*nuova stringa*/
            str = (cliManager->CLR).stringManager.newBuilder(sbString, capacity);
            assert(str != NULL);

            /*estrazione caratteri della nuova stringa*/
            strLiteral = (cliManager->CLR).stringManager.toLiteral(str);
            assert(strLiteral != NULL);

            /*estrazione caratteri della stringa del vecchio stringbuilder*/
            sbStringLiteral = (cliManager->CLR).stringManager.toLiteral(sbString);
            assert(sbStringLiteral != NULL);

            /*copia dei caratteri della vecchia stringa nella nuova, lasciando uno spazio di lunghezza length dopo index*/
            if (index > 0) {
                memcpy(strLiteral, sbStringLiteral, sizeof(JITINT16) * (index - 1));
            }
            memcpy(strLiteral + index + length, sbStringLiteral + index, sizeof(JITINT16) * (sbStringLength - index));

            /*imposta lunghezza della nuova stringa*/
            (cliManager->CLR).stringManager.setLength(str, newLength);

            /*inserisce la nuova stringa nel vecchio stringbuilder*/
            setStringFromStringBuilder(stringBuilder, str);

        } else {

            /*estrazione caratteri della stringa del vecchio stringbuilder*/
            sbStringLiteral = (cliManager->CLR).stringManager.toLiteral(sbString);
            assert(sbStringLiteral != NULL);

            /*spostamento caratteri dopo index di length posizioni avanti*/
            memmove(sbStringLiteral + index + length, sbStringLiteral + index, sizeof(JITINT16) * (sbStringLength - index));

            /*imposta lunghezza della nuova stringa*/
            (cliManager->CLR).stringManager.setLength(sbString, newLength);

        }
    }

    return stringBuilder;
}

static inline void internal_printdbgStringBuilder (void *stringBuilder, char* descrizione, JITINT8 mode) {
    void            *string;

    if (stringBuilder == NULL) {
        printf("\n %s = NULL", descrizione);
    } else {
        string = internal_getStringFromStringBuilder(stringBuilder);
        (cliManager->CLR).stringManager.printdbg(string, descrizione, mode);
    }
    printf("\n");
}

static inline void internal_lib_stringBuilder_check_metadata (void) {

    /* Check if we have to load the metadata	*/
    if (((cliManager->CLR).stringBuilderManager).StringBuilderType == NULL) {


        /* Fetch the type System.Text.StringBuilder     */
        ((cliManager->CLR).stringBuilderManager).StringBuilderType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Text", (JITINT8 *) "StringBuilder");
        assert(((cliManager->CLR).stringBuilderManager).StringBuilderType != NULL);


        /* Fetch the field buildString			*/
        ((cliManager->CLR).stringBuilderManager).buildStringField = ((cliManager->CLR).stringBuilderManager).StringBuilderType->getFieldFromName(((cliManager->CLR).stringBuilderManager).StringBuilderType, (JITINT8 *) "buildString");
        assert((((cliManager->CLR).stringBuilderManager).buildStringField) != NULL);

        /* Fetch the field maxCapacity			*/
        ((cliManager->CLR).stringBuilderManager).maxCapacityField = ((cliManager->CLR).stringBuilderManager).StringBuilderType->getFieldFromName(((cliManager->CLR).stringBuilderManager).StringBuilderType, (JITINT8 *) "maxCapacity");
        assert((((cliManager->CLR).stringBuilderManager).maxCapacityField) != NULL);

        /* Fetch the field needsCopy                    */
        ((cliManager->CLR).stringBuilderManager).needsCopyField = ((cliManager->CLR).stringBuilderManager).StringBuilderType->getFieldFromName(((cliManager->CLR).stringBuilderManager).StringBuilderType, (JITINT8 *) "needsCopy");
        assert((((cliManager->CLR).stringBuilderManager).needsCopyField) != NULL);

        /* Fetch the offset				*/
        ((cliManager->CLR).stringBuilderManager).buildStringOffset = cliManager->gc->fetchFieldOffset(((cliManager->CLR).stringBuilderManager).buildStringField);
        ((cliManager->CLR).stringBuilderManager).maxCapacityOffset = cliManager->gc->fetchFieldOffset(((cliManager->CLR).stringBuilderManager).maxCapacityField);
        ((cliManager->CLR).stringBuilderManager).needsCopyOffset = cliManager->gc->fetchFieldOffset(((cliManager->CLR).stringBuilderManager).needsCopyField);
    }

    /* Final assertions				*/
    assert(((cliManager->CLR).stringBuilderManager).StringBuilderType != NULL);
    assert((((cliManager->CLR).stringBuilderManager).buildStringField) != NULL);
    assert((((cliManager->CLR).stringBuilderManager).maxCapacityField) != NULL);
    assert((((cliManager->CLR).stringBuilderManager).needsCopyField) != NULL);
    assert(((cliManager->CLR).stringBuilderManager).buildStringOffset >= 0);
    assert(((cliManager->CLR).stringBuilderManager).maxCapacityOffset >= 0);
    assert(((cliManager->CLR).stringBuilderManager).needsCopyOffset >= 0);

    /* Return					*/
    return;
}

static inline void internal_lib_stringBuilder_initialize (void) {

    /* Initialize the metadata	*/
    PLATFORM_pthread_once(&stringBuilderMetadataLock, internal_lib_stringBuilder_check_metadata);

    /* Return			*/
    return;
}
