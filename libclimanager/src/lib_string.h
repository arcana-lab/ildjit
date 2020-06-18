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
/**
 * @file lib_string.h
 */
#ifndef LIB_STRING_H
#define LIB_STRING_H


#include <jitsystem.h>
#include <ir_method.h>
#include <metadata/metadata_types.h>
#include <metadata/streams/metadata_streams_manager.h>
#include <platform_API.h>

/**
 * @brief String manager
 *
 * This structure expose functionalities to manage System.String instances
 */
typedef struct t_stringManager {
    TypeDescriptor          *StringType;
    FieldDescriptor         *lengthField;
    FieldDescriptor         *capacityField;

    /* Description of the field storing the first string character */
    FieldDescriptor *firstCharField;

    JITINT32 lengthFieldOffset;
    JITINT32 capacityFieldOffset;
    JITINT32 firstCharFieldOffset;
    XanHashTable    *usObjects;
    XanHashTable    *internHashTable;

    TypeDescriptor *                (*fillILStringType)();                          /**< Store the information about the System.String class		*/
    JITINT32 (*getLength)(void *string);                                            /**< Return the number of UTF16 characters stored inside the System.String object given as input	*/
    JITINT32 (*getCapacity)(void *string);                                          /**< Return the capacity of the System.String object given as input	*/
    void (*setLength)(void *string, JITINT32 length);                               /**< Set the number of UTF16 characters stored inside the System.String object given as input	*/
    void (*setCapacity)(void *string, JITINT32 capacity);                           /**< Set the capacity of the System.String object given as input	*/
    JITINT16 *      (*toLiteral)(void *string);                                     /**< Convert a System.String object to the literal stored inside itself	*/
    void (*ctor_pcii)(void *string, JITINT16 *literal, JITINT32 startIndex, JITINT32 length);
    void *          (*newInstance)(JITINT16 * literal, JITINT32 length);            /**< Allocate a new instance of System.String class storing inside it the literal given as input	*/
    void *          (*newBuilder)(void *string, JITINT32 length);
    void *          (*newPermanentInstance)(JITINT16 * literal, JITINT32 length);   /**< Allocate a new instance of System.String class storing inside it the literal given as input. The object created is set to be permanent (not collectable by the garbage collector).	*/
    void *          (*newInstanceEmpty)(JITINT32 length);                           /**< Allocate a new instance of System.String class storing inside it an empty literal	*/
    void (*fromUTF8ToUTF16)(JITINT8 *literalUTF8, JITINT16 *literalUTF16, JITINT32 length);
    void (*fromUTF16ToUTF8)(JITINT16 *literalUTF16, JITINT8 *literalUTF8, JITINT32 length);
    void *          (*concatStrings)(void *string1, void *string2);                 /**< Allocate a new instance of System.String class storing inside it the concatenation of the two literals stored inside the two objects given as input	*/
    JITINT32 (*compareStrings)(void *strA, JITINT32 indexA, JITINT32 lengthA, void *strB, JITINT32 indexB, JITINT32 lengthB, JITBOOLEAN ignoreCase);
    void *          (*getUserString)(t_binary_information * binary, JITUINT32 row);
    ir_symbol_t *   (*getUserStringSymbol)(t_binary_information * binary, JITUINT32 row);
    void (*printLiteral)(void *string, JITINT8 *literalUTF8);
    void (*printdbg)(void *string, char* descrizione, JITINT8 mode);                        /*AGGIUNTA PER IL DEBUG*/
    void *          (*trim)(void *string, void *whiteSpaces, JITINT32 trimFlag);            /** trim whitespaces */
    void (*toUpper)(JITINT16 *dest, const JITINT16 *src, JITINT32 len);                     /** Unicode functions*/
    void (*toLower)(JITINT16 *dest, const JITINT16 *src, JITINT32 len);
    JITINT32 (*compareIgnoreCase)(const JITINT16 *str1, const JITINT16 *str2, JITINT32 len);
    JITINT32 (*compare)(const JITINT16 *str1, const JITINT16 *str2, JITINT32 len);
    JITUINT32 (*hashFunction)(void *string);               /**funzioni hash table stringhe**/
    JITINT32 (*hashCompare)(void *key1, void *key2);
    void *          (*intern)(void *string);
    void *          (*isInterned)(void *string);
    JITINT8*        (*getUTF8copy)(void *str);
    void (*initialize)(void);
    void (*destroy)(struct t_stringManager *self);

    /**
     * Get the offset of the first character in the string
     *
     * @return the offset of the string first character
     */
    JITINT32 (*getFirstCharOffset)(void);
} t_stringManager;

void init_stringManager (t_stringManager *stringManager);

/**
 * Allocate a new instance of System.String class storing inside it the literal given as input.
 */
void * CLIMANAGER_StringManager_newInstanceFromUTF8 (JITINT8 *literal, JITINT32 length);

#endif
